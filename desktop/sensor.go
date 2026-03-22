package main

import (
	"fmt"
	"syscall"
	"unsafe"

	"github.com/go-ole/go-ole"
)

var (
	CLSID_SensorManager       = ole.NewGUID("{77A1C827-FCD2-4689-8915-9D613CC5FA3E}")
	IID_ISensorManager        = ole.NewGUID("{BD77DB67-45A8-42DC-8D00-6DCF15F8377A}")
	SENSOR_TYPE_AMBIENT_LIGHT = ole.NewGUID("{97F115C8-599A-4153-8894-D2D12899918A}")
)

type PROPERTYKEY struct {
	Fmtid ole.GUID
	Pid   uint32
}

var SENSOR_DATA_TYPE_LIGHT_LUX = PROPERTYKEY{
	Fmtid: ole.GUID{
		Data1: 0xE4C77CE2, Data2: 0xDCB7, Data3: 0x46E9,
		Data4: [8]byte{0x84, 0x39, 0x4F, 0xEC, 0x54, 0x88, 0x33, 0xA6},
	},
	Pid: 2,
}

// vtable call helper for COM interfaces
func vtCall(obj uintptr, methodIndex int, args ...uintptr) (uintptr, error) {
	vtbl := *(*uintptr)(unsafe.Pointer(obj))
	method := *(*uintptr)(unsafe.Pointer(vtbl + uintptr(methodIndex)*unsafe.Sizeof(vtbl)))
	allArgs := append([]uintptr{obj}, args...)
	ret, _, err := syscall.SyscallN(method, allArgs...)
	if ret != 0 {
		return ret, fmt.Errorf("HRESULT 0x%08X: %v", ret, err)
	}
	return 0, nil
}

func vtRelease(obj uintptr) {
	if obj != 0 {
		vtCall(obj, 2)
	}
}

func readLuxFromSensorAPI() (float64, error) {
	mgr, err := ole.CreateInstance(CLSID_SensorManager, IID_ISensorManager)
	if err != nil {
		return 0, fmt.Errorf("SensorManager: %v", err)
	}
	mgrPtr := uintptr(unsafe.Pointer(mgr))
	defer vtRelease(mgrPtr)

	var collPtr uintptr
	_, err = vtCall(mgrPtr, 4,
		uintptr(unsafe.Pointer(SENSOR_TYPE_AMBIENT_LIGHT)),
		uintptr(unsafe.Pointer(&collPtr)),
	)
	if err != nil {
		return 0, fmt.Errorf("GetSensorsByType: %v", err)
	}
	defer vtRelease(collPtr)

	var count uint32
	_, err = vtCall(collPtr, 4, uintptr(unsafe.Pointer(&count)))
	if err != nil {
		return 0, fmt.Errorf("GetCount: %v", err)
	}
	if count == 0 {
		return 0, fmt.Errorf("no ambient light sensors found")
	}

	var sensorPtr uintptr
	_, err = vtCall(collPtr, 3, 0, uintptr(unsafe.Pointer(&sensorPtr)))
	if err != nil {
		return 0, fmt.Errorf("GetAt: %v", err)
	}
	defer vtRelease(sensorPtr)

	var reportPtr uintptr
	_, err = vtCall(sensorPtr, 13, uintptr(unsafe.Pointer(&reportPtr)))
	if err != nil {
		return 0, fmt.Errorf("GetData: %v", err)
	}
	defer vtRelease(reportPtr)

	var propVar [24]byte
	_, err = vtCall(reportPtr, 4,
		uintptr(unsafe.Pointer(&SENSOR_DATA_TYPE_LIGHT_LUX)),
		uintptr(unsafe.Pointer(&propVar[0])),
	)
	if err != nil {
		return 0, fmt.Errorf("GetSensorValue: %v", err)
	}

	vt := *(*uint16)(unsafe.Pointer(&propVar[0]))
	switch vt {
	case 4: // VT_R4
		return float64(*(*float32)(unsafe.Pointer(&propVar[8]))), nil
	case 5: // VT_R8
		return *(*float64)(unsafe.Pointer(&propVar[8])), nil
	case 19: // VT_UI4
		return float64(*(*uint32)(unsafe.Pointer(&propVar[8]))), nil
	case 3: // VT_I4
		return float64(*(*int32)(unsafe.Pointer(&propVar[8]))), nil
	default:
		return 0, fmt.Errorf("unexpected PROPVARIANT type: %d", vt)
	}
}
