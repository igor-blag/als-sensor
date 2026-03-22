package main

import (
	"encoding/json"
	"math"
	"os"
	"path/filepath"
	"sort"
	"sync"
	"time"
)

type CalibrationPoint struct {
	Lux        float64 `json:"lux"`
	Brightness int     `json:"brightness"`
}

type CalibrationStore struct {
	Points  []CalibrationPoint `json:"points"`
	Updated time.Time          `json:"updated"`
	mu      sync.Mutex
}

func loadCalibration() *CalibrationStore {
	cal := &CalibrationStore{}
	data, err := os.ReadFile(filepath.Join(exeDir(), "calibration.json"))
	if err != nil {
		return cal
	}
	json.Unmarshal(data, cal)
	sort.Slice(cal.Points, func(i, j int) bool { return cal.Points[i].Lux < cal.Points[j].Lux })
	return cal
}

func (c *CalibrationStore) Save() error {
	c.mu.Lock()
	defer c.mu.Unlock()
	data, err := json.MarshalIndent(c, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(filepath.Join(exeDir(), "calibration.json"), data, 0644)
}

// AddPoint records a user-preferred brightness at a given lux level.
// If a nearby point exists (within 10% lux), it replaces it.
// Max 50 points.
func (c *CalibrationStore) AddPoint(lux float64, brightness int) {
	c.mu.Lock()
	defer c.mu.Unlock()

	// Replace nearby point
	for i, p := range c.Points {
		if math.Abs(p.Lux-lux) < lux*0.1+1 {
			c.Points[i] = CalibrationPoint{Lux: lux, Brightness: brightness}
			c.Updated = time.Now()
			sort.Slice(c.Points, func(a, b int) bool { return c.Points[a].Lux < c.Points[b].Lux })
			return
		}
	}

	c.Points = append(c.Points, CalibrationPoint{Lux: lux, Brightness: brightness})
	sort.Slice(c.Points, func(a, b int) bool { return c.Points[a].Lux < c.Points[b].Lux })

	// Limit to 50 points — remove oldest middle point
	if len(c.Points) > 50 {
		c.Points = append(c.Points[:25], c.Points[26:]...)
	}

	c.Updated = time.Now()
}

func (c *CalibrationStore) Reset() {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.Points = nil
	c.Updated = time.Now()
}
