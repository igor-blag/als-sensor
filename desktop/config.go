package main

import (
	"encoding/json"
	"os"
	"path/filepath"
)

type Config struct {
	PollIntervalMs   int     `json:"poll_interval_ms"`
	AlphaUp          float64 `json:"smoothing_alpha_up"`
	AlphaDown        float64 `json:"smoothing_alpha_down"`
	BrightnessMin    int     `json:"brightness_min"`
	BrightnessMax    int     `json:"brightness_max"`
	SafetyThreshold  int     `json:"safety_threshold"`
	SafetyTimeoutSec int     `json:"safety_timeout_sec"`
	CalibrationDetectIntervalSec int `json:"calibration_detect_interval_sec"`
	CalibrationDetectCooldownSec int `json:"calibration_detect_cooldown_sec"`
}

var defaultConfig = Config{
	PollIntervalMs:   1000,
	AlphaUp:          0.4,
	AlphaDown:        0.08,
	BrightnessMin:    0,
	BrightnessMax:    100,
	SafetyThreshold:  5,
	SafetyTimeoutSec: 5,
	CalibrationDetectIntervalSec: 10,
	CalibrationDetectCooldownSec: 3,
}

func exeDir() string {
	exe, err := os.Executable()
	if err != nil {
		return "."
	}
	return filepath.Dir(exe)
}

func loadConfig() Config {
	cfg := defaultConfig
	data, err := os.ReadFile(filepath.Join(exeDir(), "config.json"))
	if err != nil {
		return cfg
	}
	json.Unmarshal(data, &cfg)
	return cfg
}

func saveConfig(cfg Config) error {
	data, err := json.MarshalIndent(cfg, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(filepath.Join(exeDir(), "config.json"), data, 0644)
}
