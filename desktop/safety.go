package main

import (
	"runtime"
	"syscall"
	"unsafe"

	"golang.org/x/sys/windows"
)

var (
	procMessageBoxTimeoutW = windows.NewLazySystemDLL("user32.dll").NewProc("MessageBoxTimeoutW")
)

const (
	MB_YESNO          = 0x00000004
	MB_ICONWARNING    = 0x00000030
	MB_SETFOREGROUND  = 0x00010000
	IDYES             = 6
	IDTIMEOUT         = 32000
)

// confirmLowBrightness shows a dialog asking the user to confirm low brightness.
// Returns true if user confirms, false if timeout or rejected.
func confirmLowBrightness(brightness int, timeoutSec int) bool {
	// Must run on a dedicated OS thread for Win32 message pump
	result := make(chan bool, 1)
	go func() {
		runtime.LockOSThread()
		defer runtime.UnlockOSThread()

		title, _ := syscall.UTF16PtrFromString("ALS Brightness")
		text, _ := syscall.UTF16PtrFromString(
			"Яркость установлена на " + itoa(brightness) + "%.\n" +
				"Оставить? Через " + itoa(timeoutSec) + " сек. вернётся прежняя яркость.",
		)

		ret, _, _ := procMessageBoxTimeoutW.Call(
			0,
			uintptr(unsafe.Pointer(text)),
			uintptr(unsafe.Pointer(title)),
			MB_YESNO|MB_ICONWARNING|MB_SETFOREGROUND,
			0,
			uintptr(timeoutSec*1000),
		)

		result <- (ret == IDYES)
	}()
	return <-result
}

func itoa(n int) string {
	if n == 0 {
		return "0"
	}
	s := ""
	neg := false
	if n < 0 {
		neg = true
		n = -n
	}
	for n > 0 {
		s = string(rune('0'+n%10)) + s
		n /= 10
	}
	if neg {
		s = "-" + s
	}
	return s
}
