package main

import (
	"fmt"
	"log"
	"unsafe"

	"golang.org/x/sys/windows"
)

var (
	dxva2  = windows.NewLazySystemDLL("dxva2.dll")
	user32 = windows.NewLazySystemDLL("user32.dll")

	procGetNumberOfPhysicalMonitorsFromHMONITOR = dxva2.NewProc("GetNumberOfPhysicalMonitorsFromHMONITOR")
	procGetPhysicalMonitorsFromHMONITOR         = dxva2.NewProc("GetPhysicalMonitorsFromHMONITOR")
	procSetVCPFeature                           = dxva2.NewProc("SetVCPFeature")
	procGetVCPFeatureAndVCPFeatureReply         = dxva2.NewProc("GetVCPFeatureAndVCPFeatureReply")
	procEnumDisplayMonitors                     = user32.NewProc("EnumDisplayMonitors")
)

const VCP_BRIGHTNESS = 0x10

type PHYSICAL_MONITOR struct {
	hPhysicalMonitor             uintptr
	szPhysicalMonitorDescription [128]uint16
}

type monitorInfo struct {
	physical uintptr
	name     string
}

func getMonitors() ([]monitorInfo, error) {
	var monitors []monitorInfo

	callback := windows.NewCallback(func(hMonitor, hdcMonitor, lprcMonitor, dwData uintptr) uintptr {
		var numMonitors uint32
		ret, _, _ := procGetNumberOfPhysicalMonitorsFromHMONITOR.Call(hMonitor, uintptr(unsafe.Pointer(&numMonitors)))
		if ret == 0 || numMonitors == 0 {
			return 1
		}
		physMons := make([]PHYSICAL_MONITOR, numMonitors)
		ret, _, _ = procGetPhysicalMonitorsFromHMONITOR.Call(hMonitor, uintptr(numMonitors), uintptr(unsafe.Pointer(&physMons[0])))
		if ret != 0 {
			for _, pm := range physMons {
				name := windows.UTF16ToString(pm.szPhysicalMonitorDescription[:])
				log.Printf("  Found physical monitor: handle=%v name=%q", pm.hPhysicalMonitor, name)

				// Test if DDC/CI actually works on this monitor
				var cur, max uint32
				testRet, _, _ := procGetVCPFeatureAndVCPFeatureReply.Call(
					pm.hPhysicalMonitor, VCP_BRIGHTNESS, 0,
					uintptr(unsafe.Pointer(&cur)),
					uintptr(unsafe.Pointer(&max)),
				)
				if testRet != 0 && max > 0 {
					log.Printf("    DDC/CI OK: brightness=%d/%d", cur, max)
					monitors = append(monitors, monitorInfo{physical: pm.hPhysicalMonitor, name: name})
				} else {
					log.Printf("    DDC/CI not available (ret=%d max=%d), skipping", testRet, max)
				}
			}
		}
		return 1
	})
	procEnumDisplayMonitors.Call(0, 0, callback, 0)
	if len(monitors) == 0 {
		return nil, fmt.Errorf("no DDC/CI monitors found")
	}
	return monitors, nil
}

func setBrightness(physical uintptr, brightness int) error {
	ret, _, err := procSetVCPFeature.Call(physical, VCP_BRIGHTNESS, uintptr(brightness))
	if ret == 0 {
		return fmt.Errorf("SetVCPFeature(handle=%v, brightness=%d): %v", physical, brightness, err)
	}
	return nil
}

func getBrightness(physical uintptr) (int, error) {
	var cur, max uint32
	ret, _, err := procGetVCPFeatureAndVCPFeatureReply.Call(
		physical, VCP_BRIGHTNESS, 0,
		uintptr(unsafe.Pointer(&cur)),
		uintptr(unsafe.Pointer(&max)),
	)
	if ret == 0 {
		return 0, fmt.Errorf("GetVCPFeatureAndVCPFeatureReply: %v", err)
	}
	return int(cur), nil
}
