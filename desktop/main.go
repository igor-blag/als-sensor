package main

import (
	"log"
	"math"
	"os"
	"path/filepath"
	"sync"
	"time"

	"github.com/go-ole/go-ole"
)

func main() {
	// Log to file for diagnostics
	logFile, err := os.OpenFile(filepath.Join(exeDir(), "als-brightness.log"), os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
	if err == nil {
		log.SetOutput(logFile)
		defer logFile.Close()
	}
	log.Println("Starting ALS Brightness")

	ole.CoInitializeEx(0, ole.COINIT_MULTITHREADED)
	defer ole.CoUninitialize()

	// Start sensor loop in background, tray on main thread
	go sensorLoop()
	setupTray()
}

func sensorLoop() {
	<-trayReady

	ole.CoInitializeEx(0, ole.COINIT_MULTITHREADED)

	cfg := loadConfig()
	cal := loadCalibration()

	lux, err := readLuxFromSensorAPI()
	if err != nil {
		log.Printf("Sensor error: %v", err)
		updateTrayTooltip(0, 0, false)
		return
	}
	log.Printf("Sensor OK, lux=%.1f", lux)

	monitors, err := getMonitors()
	if err != nil {
		log.Printf("Monitor error: %v", err)
		updateTrayTooltip(0, 0, false)
		return
	}
	log.Printf("Found %d DDC/CI monitor(s)", len(monitors))

	var (
		lastBrightness int     = -1
		smoothLux      float64 = lux
		paused         bool
		lastSetTime    time.Time
		safetyPending  bool
		mu             sync.Mutex
	)

	// Calibration detection goroutine
	calTicker := time.NewTicker(time.Duration(cfg.CalibrationDetectIntervalSec) * time.Second)
	defer calTicker.Stop()

	go func() {
		for {
			select {
			case <-trayQuitCh:
				return
			case <-calTicker.C:
				if paused || len(monitors) == 0 {
					continue
				}
				mu.Lock()
				sinceSet := time.Since(lastSetTime)
				currentSmooth := smoothLux
				lastSet := lastBrightness
				mu.Unlock()

				if sinceSet < time.Duration(cfg.CalibrationDetectCooldownSec)*time.Second {
					continue
				}

				actual, err := getBrightness(monitors[0].physical)
				if err != nil {
					continue
				}

				if lastSet >= 0 && int(math.Abs(float64(actual-lastSet))) > 2 {
					log.Printf("Calibration: user adjusted %d -> %d at lux=%.1f", lastSet, actual, currentSmooth)
					cal.AddPoint(currentSmooth, actual)
					cal.Save()
					mu.Lock()
					lastBrightness = actual
					mu.Unlock()
					updateTrayTooltip(currentSmooth, actual, paused)
				}
			case <-trayResetCal:
				cal.Reset()
				cal.Save()
				log.Println("Calibration reset")
			}
		}
	}()

	ticker := time.NewTicker(time.Duration(cfg.PollIntervalMs) * time.Millisecond)
	defer ticker.Stop()

	for {
		select {
		case <-trayQuitCh:
			return
		case <-trayPauseCh:
			paused = !paused
			updateTrayTooltip(smoothLux, lastBrightness, paused)
			continue
		case <-ticker.C:
		}

		if paused {
			continue
		}

		lux, err := readLuxFromSensorAPI()
		if err != nil {
			continue
		}

		mu.Lock()
		smoothLux = smoothLuxValue(smoothLux, lux, cfg)
		target := luxToBrightness(smoothLux, cfg, cal)

		// Dead-zone: skip if change < 2%
		if lastBrightness >= 0 && int(math.Abs(float64(target-lastBrightness))) < 2 {
			mu.Unlock()
			updateTrayTooltip(smoothLux, lastBrightness, paused)
			continue
		}

		// Safety check for low brightness
		if target < cfg.SafetyThreshold && lastBrightness >= cfg.SafetyThreshold && !safetyPending {
			safetyPending = true
			prev := lastBrightness
			mu.Unlock()

			for _, mon := range monitors {
				setBrightness(mon.physical, target)
			}

			confirmed := confirmLowBrightness(target, cfg.SafetyTimeoutSec)
			mu.Lock()
			if confirmed {
				lastBrightness = target
			} else {
				for _, mon := range monitors {
					setBrightness(mon.physical, prev)
				}
				lastBrightness = prev
				smoothLux = float64(cfg.SafetyThreshold + 1)
			}
			lastSetTime = time.Now()
			safetyPending = false
			mu.Unlock()
			updateTrayTooltip(smoothLux, lastBrightness, paused)
			continue
		}

		log.Printf("lux=%.1f smooth=%.1f -> brightness=%d%%", lux, smoothLux, target)
		for _, mon := range monitors {
			if err := setBrightness(mon.physical, target); err != nil {
				log.Printf("DDC/CI error: %v", err)
			}
		}
		lastBrightness = target
		lastSetTime = time.Now()
		mu.Unlock()

		updateTrayTooltip(smoothLux, target, paused)
	}
}
