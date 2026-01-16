# Makefile for ESP32 PlatformIO Template

PLATFORMIO ?= pio
BOARD ?= esp32c3
PYTHON ?= python3

# Optional "argument" after flash/monitor/run (e.g. "make flash 1")
ACTION_TARGETS := flash monitor run deploy-fs
ifneq ($(filter $(ACTION_TARGETS),$(firstword $(MAKECMDGOALS))),)
  ARG := $(word 2,$(MAKECMDGOALS))
  ifneq ($(ARG),)
    NR := $(ARG)
    # Create a dummy target so the number (e.g. "1") is not a real target
    $(eval $(ARG):;@:)
  endif
endif

# Optional flags based on NR
ifdef NR
  UPLOAD_FLAG := --upload-port /dev/ttyACM$(NR)
  MONITOR_FLAG := --port /dev/ttyACM$(NR)
else
  UPLOAD_FLAG :=
  MONITOR_FLAG :=
endif

.PHONY: all build flash monitor run clean list deploy-web deploy-fs deploy-flash web-headers help

# Default target
all: build

# Help target
help:
	@echo "ESP32 PlatformIO Template - Makefile Commands"
	@echo "=============================================="
	@echo ""
	@echo "Building:"
	@echo "  make build              Build firmware (includes web-headers)"
	@echo "  make web-headers        Build web UI and generate C headers"
	@echo "  make clean              Clean build artifacts"
	@echo ""
	@echo "Flashing:"
	@echo "  make flash              Flash firmware (auto-detect port)"
	@echo "  make flash 1            Flash to /dev/ttyACM1"
	@echo "  make deploy-fs          Upload filesystem (SPIFFS/LittleFS)"
	@echo "  make deploy-fs 2        Upload filesystem to /dev/ttyACM2"
	@echo ""
	@echo "Monitoring:"
	@echo "  make monitor            Serial monitor (auto-detect port)"
	@echo "  make monitor 1          Serial monitor on /dev/ttyACM1"
	@echo ""
	@echo "Combined:"
	@echo "  make run                Build + Flash + Monitor"
	@echo "  make run 1              Build + Flash + Monitor on /dev/ttyACM1"
	@echo "  make deploy-flash       Web + Firmware + Filesystem (complete)"
	@echo ""
	@echo "Web Deployment:"
	@echo "  make deploy-web         Deploy web to data-template/"
	@echo ""
	@echo "Tools:"
	@echo "  make list               List connected ESP32 devices"
	@echo ""
	@echo "Release:"
	@echo "  make release v=1.0.0          Create tagged release"
	@echo ""
	@echo "Board Selection:"
	@echo "  BOARD=esp32   (default)  ESP32 Generic"
	@echo "  BOARD=esp32c3            ESP32-C3"
	@echo "  BOARD=esp32s3            ESP32-S3"
	@echo "  BOARD=esp32c6            ESP32-C6"
	@echo ""
	@echo "Examples:"
	@echo "  make build BOARD=esp32c3"
	@echo "  make flash 1 BOARD=esp32c3"
	@echo "  make run 2 BOARD=esp32s3"

# Build firmware (includes web headers)
build: web-headers
	@echo "🔨 Building firmware for $(BOARD)..."
	$(PLATFORMIO) run --environment $(BOARD)
	@echo "✅ Build complete"

# Build web interface and generate C headers
web-headers:
	@echo "🌐 Building web UI (Vite)..."
	@cd web && npm install --silent
	@cd web && npm run build
	@echo "🗜️  Converting web files to gzipped C headers..."
	@$(PYTHON) tools/web-to-header.py web/dist -o lib/WebService
	@echo "✅ Headers generated in lib/WebService/web_files.h"

# Flash firmware
# make flash        -> without --upload-port (auto-detect)
# make flash 1      -> Upload auf /dev/ttyACM1
flash:
	@echo "📤 Flashing firmware to $(BOARD)..."
	$(PLATFORMIO) run --target upload --environment $(BOARD) $(UPLOAD_FLAG)
	@echo "✅ Firmware flashed"

# Serial monitor
# make monitor      -> without --port (auto-detect)
# make monitor 2    -> Monitor auf /dev/ttyACM2
monitor:
	@echo "📟 Starting serial monitor for $(BOARD)..."
	$(PLATFORMIO) device monitor --environment $(BOARD) $(MONITOR_FLAG)

# Build, flash, and monitor
# make run          -> flash then monitor (without port)
# make run 1        -> flash/monitor auf /dev/ttyACM1
run: build flash monitor

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	$(PLATFORMIO) run --target clean --environment $(BOARD)
	@cd web && rm -rf dist node_modules
	@rm -f lib/WebService/web_files.h
	@echo "✅ Clean complete"

# Web Interface Deployment Targets
# =================================

# Deploy web interface from /web/dist/ to /data-template/
deploy-web:
	@echo "🚀 Deploying web interface to data-template/..."
	@./tools/deploy-web.sh
	@echo "✅ Web deployment complete"

# Upload filesystem (data-template/) to ESP32
deploy-fs:
	@echo "📁 Uploading filesystem to ESP32..."
	$(PLATFORMIO) run --target uploadfs --environment $(BOARD) $(UPLOAD_FLAG)
	@echo "✅ Filesystem uploaded"

# Deploy web interface and flash ESP32 with firmware + filesystem
deploy-flash: deploy-web build flash deploy-fs
	@echo ""
	@echo "🎉 Complete deployment finished!"
	@echo "✅ Web interface deployed to data-template/"
	@echo "✅ Firmware flashed"
	@echo "✅ Filesystem uploaded"

# List connected ESP32 devices
# make list         -> only ESP devices on /dev/ttyACM<N> with numbers (no duplicates)
list:
	@echo "Connected ESP32 Devices:"
	@echo "NR  PORT          DESCRIPTION"
	@echo "--- ------------- --------------------------------------------------"
	@$(PLATFORMIO) device list --json-output 2>/dev/null | jq -r 'map(select(((.hwid // "") | test("VID:PID=303A:|VID:PID=10C4:|VID:PID=1A86:", "i")) or ((.description // "") | test("Espressif|USB JTAG/serial|CP210|CH340", "i")))) | map(select(.port | test("^/dev/tty(ACM|USB)[0-9]+"))) | unique_by(.port) | .[] | (if (.port | test("ACM")) then (.port | capture("ACM(?<n>[0-9]+)").n) else (.port | capture("USB(?<n>[0-9]+)").n) end) + "   " + .port + "  " + (.description // "")' || echo "No devices found or jq not installed"

# Release Management
# ==================

.PHONY: release
release:
	@if [ -z "$(v)" ] && [ -z "$(VERSION)" ]; then echo "❌ VERSION required (use make release v=1.2.3 or VERSION=v1.2.3)"; exit 1; fi
	$(eval VERSION_INPUT := $(if $(v),$(v),$(VERSION)))
	$(eval VERSION_CLEAN := $(shell echo "$(VERSION_INPUT)" | sed 's/^v//'))
	@echo "🚀 Starting release v$(VERSION_CLEAN)..."
	@echo "// v$(VERSION_CLEAN)" > VERSION
	@$(PYTHON) scripts/inject_version.py
	@$(MAKE) web-headers
	@$(PLATFORMIO) run --environment $(BOARD)
	@echo "📝 Preparing release commit..."
	@echo "Release v$(VERSION_CLEAN)" > /tmp/release_msg.txt
	@echo "" >> /tmp/release_msg.txt
	@last_tag=$$(git describe --tags --abbrev=0 2>/dev/null || echo ""); \
	if [ -n "$$last_tag" ]; then \
		commit_count=$$(git rev-list $$last_tag..HEAD --count); \
		if [ $$commit_count -eq 0 ]; then \
			echo "No changes since last release ($$last_tag)" >> /tmp/release_msg.txt; \
		else \
			echo "Changes since $$last_tag:" >> /tmp/release_msg.txt; \
			git log $$last_tag..HEAD --format="- %s" -20 >> /tmp/release_msg.txt; \
		fi; \
	else \
		echo "Recent changes:" >> /tmp/release_msg.txt; \
		git log -5 --format="- %s" >> /tmp/release_msg.txt; \
	fi
	@git add VERSION include/version.h lib/WebService/web_files.h
	@git commit -F /tmp/release_msg.txt
	@rm /tmp/release_msg.txt
	@git tag v$(VERSION_CLEAN)
	@echo "📤 Pushing to origin..."
	@git push origin HEAD
	@git push origin v$(VERSION_CLEAN)
	@echo "✅ Release v$(VERSION_CLEAN) published"
