package main

import (
	"math"
	"sort"
)

// luxToBrightness converts lux to brightness percentage using calibration curve
// or default logarithmic curve as fallback
func luxToBrightness(lux float64, cfg Config, cal *CalibrationStore) int {
	var b float64

	if cal != nil && len(cal.Points) >= 2 {
		b = calibratedBrightness(lux, cfg, cal)
	} else {
		b = defaultCurve(lux, cfg)
	}

	brightness := int(math.Round(b))
	if brightness < cfg.BrightnessMin {
		brightness = cfg.BrightnessMin
	}
	if brightness > cfg.BrightnessMax {
		brightness = cfg.BrightnessMax
	}
	return brightness
}

func defaultCurve(lux float64, cfg Config) float64 {
	if lux <= 0 {
		return float64(cfg.BrightnessMin)
	}
	if lux >= 1000 {
		return float64(cfg.BrightnessMax)
	}
	minB := float64(cfg.BrightnessMin)
	maxB := float64(cfg.BrightnessMax)
	return minB + (maxB-minB)*(math.Log10(lux+1)/math.Log10(1001))
}

func calibratedBrightness(lux float64, cfg Config, cal *CalibrationStore) float64 {
	points := cal.Points
	n := len(points)

	// Below lowest calibration point — blend with default curve
	if lux <= points[0].Lux {
		return defaultCurve(lux, cfg)
	}
	// Above highest calibration point — blend with default curve
	if lux >= points[n-1].Lux {
		return defaultCurve(lux, cfg)
	}

	// Find surrounding calibration points and interpolate
	i := sort.Search(n, func(j int) bool { return points[j].Lux > lux }) - 1
	if i < 0 {
		i = 0
	}
	if i >= n-1 {
		return float64(points[n-1].Brightness)
	}

	p0, p1 := points[i], points[i+1]
	t := (lux - p0.Lux) / (p1.Lux - p0.Lux)
	return float64(p0.Brightness)*(1-t) + float64(p1.Brightness)*t
}

// smoothLux applies asymmetric exponential moving average
func smoothLuxValue(current, newVal float64, cfg Config) float64 {
	if current < 0 {
		return newVal
	}
	if newVal > current {
		return current*(1-cfg.AlphaUp) + newVal*cfg.AlphaUp
	}
	return current*(1-cfg.AlphaDown) + newVal*cfg.AlphaDown
}
