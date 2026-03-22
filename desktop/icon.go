package main

// 16x16 ICO file — simple brightness icon (yellow circle on transparent background)
// Generated programmatically as a minimal valid ICO
var iconData = func() []byte {
	// Build a 16x16 32-bit BGRA ICO
	w, h := 16, 16
	pixels := make([]byte, w*h*4)

	// Draw a filled circle (sun icon)
	cx, cy, r := 7.5, 7.5, 6.0
	for y := 0; y < h; y++ {
		for x := 0; x < w; x++ {
			dx := float64(x) - cx
			dy := float64(y) - cy
			dist := dx*dx + dy*dy
			off := ((h-1-y)*w + x) * 4 // ICO is bottom-up

			if dist <= r*r {
				// Yellow sun: BGRA
				pixels[off+0] = 0x00 // B
				pixels[off+1] = 0xCC // G
				pixels[off+2] = 0xFF // R
				pixels[off+3] = 0xFF // A
			} else if dist <= (r+1)*(r+1) {
				// Anti-alias edge: orange
				pixels[off+0] = 0x00 // B
				pixels[off+1] = 0x99 // G
				pixels[off+2] = 0xFF // R
				pixels[off+3] = 0x80 // A (semi-transparent)
			}
			// else transparent (0,0,0,0)
		}
	}

	// ICO file format
	ico := []byte{
		0, 0, // reserved
		1, 0, // type: ICO
		1, 0, // count: 1 image
	}
	// ICONDIRENTRY (16 bytes)
	dataOffset := 6 + 16 // header + 1 entry
	bmpSize := 40 + len(pixels)
	ico = append(ico,
		byte(w), byte(h), // width, height (0 means 256, but 16 is fine)
		0,                 // color palette
		0,                 // reserved
		1, 0,              // color planes
		32, 0,             // bits per pixel
		byte(bmpSize), byte(bmpSize>>8), byte(bmpSize>>16), byte(bmpSize>>24),
		byte(dataOffset), byte(dataOffset>>8), byte(dataOffset>>16), byte(dataOffset>>24),
	)
	// BITMAPINFOHEADER (40 bytes)
	ico = append(ico,
		40, 0, 0, 0, // biSize
		byte(w), 0, 0, 0, // biWidth
		byte(h*2), 0, 0, 0, // biHeight (doubled for ICO)
		1, 0, // biPlanes
		32, 0, // biBitCount
		0, 0, 0, 0, // biCompression
		0, 0, 0, 0, // biSizeImage
		0, 0, 0, 0, // biXPelsPerMeter
		0, 0, 0, 0, // biYPelsPerMeter
		0, 0, 0, 0, // biClrUsed
		0, 0, 0, 0, // biClrImportant
	)
	ico = append(ico, pixels...)

	return ico
}()
