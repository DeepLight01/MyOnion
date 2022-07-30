.PHONY: all setup build build_apps build_external package release clean

###########################################################

VERSION=3.13.0

###########################################################

TARGET=Onion
RELEASE_NAME=$(TARGET)-v$(VERSION)
	
###########################################################

ROOT_DIR := $(shell pwd -P)
SRC_DIR := $(ROOT_DIR)/src
THIRD_PARTY_DIR := $(ROOT_DIR)/third-party
BUILD_DIR := $(ROOT_DIR)/build
PACKAGE_DIR := $(ROOT_DIR)/package
RELEASE_DIR := $(ROOT_DIR)/release
STATIC_BUILD := $(ROOT_DIR)/static/build
STATIC_PACKAGE := $(ROOT_DIR)/static/package

###########################################################

all: clean setup build build_apps build_external package

.build_cached:
	@make all
	@touch .build_cached

setup:
	@echo :: $(TARGET) - setup
	@mkdir -p $(BUILD_DIR) $(PACKAGE_DIR) $(RELEASE_DIR)
	@cp -R $(STATIC_BUILD)/. $(BUILD_DIR)
	@cp -R $(STATIC_PACKAGE)/. $(PACKAGE_DIR)
	@cp -R $(ROOT_DIR)/lib/. $(BUILD_DIR)/.tmp_update/lib
	@echo -n " v$(VERSION)" > $(BUILD_DIR)/.tmp_update/onionVersion/version.txt
	@$(ROOT_DIR)/get_themes.sh

build:
	@echo :: $(TARGET) - build
	@cd $(SRC_DIR)/bootScreen && BUILD_DIR=$(BUILD_DIR)/.tmp_update make
	@cd $(SRC_DIR)/chargingState && BUILD_DIR=$(BUILD_DIR)/.tmp_update make
	@cd $(SRC_DIR)/checkCharge && BUILD_DIR=$(BUILD_DIR)/.tmp_update make
	@cd $(SRC_DIR)/gameSwitcher && BUILD_DIR=$(BUILD_DIR)/.tmp_update make
	@cd $(SRC_DIR)/lastGame && BUILD_DIR=$(BUILD_DIR)/.tmp_update make
	@cd $(SRC_DIR)/mainUiBatPerc && BUILD_DIR=$(BUILD_DIR)/.tmp_update make
	@cd $(SRC_DIR)/onionKeymon && BUILD_DIR=$(BUILD_DIR)/.tmp_update make

build_apps:
	@echo :: $(TARGET) - build apps
	@cd $(SRC_DIR)/playActivity && BUILD_DIR=$(BUILD_DIR)/App/PlayActivity make
	@cd $(SRC_DIR)/playActivityUI && BUILD_DIR=$(BUILD_DIR)/App/PlayActivity make
	@cd $(SRC_DIR)/onionInstaller && BUILD_DIR=$(BUILD_DIR)/App/The_Onion_Installer make
	@cd $(SRC_DIR)/themeSwitcher && BUILD_DIR=$(BUILD_DIR)/App/The_Onion_Installer/data/Layer2/ThemeSwitcher/App/ThemeSwitcher make

build_external:
	@echo :: $(TARGET) - build external
	@cd $(THIRD_PARTY_DIR)/RetroArch && make && cp retroarch $(BUILD_DIR)/RetroArch/
	@cd $(THIRD_PARTY_DIR)/SearchFilter && make && cp -a build/. "$(BUILD_DIR)/App/The_Onion_Installer/data/Layer2/Search and Filter/"

package:
	@echo :: $(TARGET) - package
	@cd $(BUILD_DIR) && zip -rq $(PACKAGE_DIR)/miyoo/app/.installer/onion_package.zip .
	@cd $(SRC_DIR)/installUI && BUILD_DIR=$(PACKAGE_DIR)/miyoo/app/.installer make

release: .build_cached
	@echo :: $(TARGET) - release
	@cd $(PACKAGE_DIR) && zip -rq $(RELEASE_DIR)/$(RELEASE_NAME).zip .
	@rm -f .build_cached

clean:
	@rm -rf $(BUILD_DIR) $(PACKAGE_DIR)
	@rm -f $(PACKAGE_DIR)/$(RELEASE_NAME).zip
	@rm -f .build_cached
	@find include src -type f -name *.o -exec rm -f {} \;
	@echo :: $(TARGET) - cleaned