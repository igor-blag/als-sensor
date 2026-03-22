package main

import (
	"fmt"

	"github.com/energye/systray"
)

var (
	trayReady    = make(chan struct{})
	trayTooltip  = make(chan string, 1)
	trayPaused   bool
	trayPauseCh  = make(chan struct{}, 1)
	trayQuitCh   = make(chan struct{})
	trayResetCal = make(chan struct{}, 1)
)

func setupTray() {
	systray.Run(onTrayReady, onTrayExit)
}

func onTrayReady() {
	systray.SetIcon(iconData)
	systray.SetTitle("")
	systray.SetTooltip("ALS Brightness")

	// Both left and right click open the menu
	systray.SetOnClick(func(menu systray.IMenu) {
		menu.ShowMenu()
	})

	mPause := systray.AddMenuItem("Пауза", "Приостановить автояркость")
	systray.AddSeparator()
	mResetCal := systray.AddMenuItem("Сбросить калибровку", "Удалить сохранённую кривую яркости")
	systray.AddSeparator()
	mQuit := systray.AddMenuItem("Выход", "Закрыть ALS Brightness")

	mPause.Click(func() {
		trayPaused = !trayPaused
		if trayPaused {
			mPause.SetTitle("Возобновить")
			mPause.SetTooltip("Возобновить автояркость")
		} else {
			mPause.SetTitle("Пауза")
			mPause.SetTooltip("Приостановить автояркость")
		}
		select {
		case trayPauseCh <- struct{}{}:
		default:
		}
	})

	mResetCal.Click(func() {
		select {
		case trayResetCal <- struct{}{}:
		default:
		}
	})

	mQuit.Click(func() {
		close(trayQuitCh)
		systray.Quit()
	})

	close(trayReady)

	// Tooltip updater
	go func() {
		for tip := range trayTooltip {
			systray.SetTooltip(tip)
		}
	}()
}

func onTrayExit() {}

func updateTrayTooltip(lux float64, brightness int, paused bool) {
	tip := fmt.Sprintf("ALS: %.0f lux | %d%%", lux, brightness)
	if paused {
		tip += " (пауза)"
	}
	select {
	case trayTooltip <- tip:
	default:
		select {
		case <-trayTooltip:
		default:
		}
		trayTooltip <- tip
	}
}
