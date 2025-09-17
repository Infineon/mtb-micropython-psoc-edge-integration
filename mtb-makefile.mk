
MTB_LIBS_DIR = $(TOP)/lib/mtb-psoc-edge-libs

mtb_init: mtb_deinit mtb_add_bsp mtb_set_bsp mtb_get_libs
	$(info )
	$(info Initializing ModusToolbox libs for board $(BOARD))

mtb_get_libs:
	$(info )
	$(info Retrieving ModusToolbox dependencies ...)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) getlibs

mtb_add_bsp:
	$(info )
	$(info Adding board $(BOARD) dependencies ...)
	$(Q) cd $(MTB_LIBS_DIR); library-manager-cli --project . --add-bsp-name $(BOARD) --add-bsp-version $(BOARD_VERSION)

mtb_set_bsp: 
	$(info )
	$(info Setting board $(BOARD) as active ...)
	$(Q) cd $(MTB_LIBS_DIR); library-manager-cli --project . --set-active-bsp APP_$(BOARD)

# Remove MTB retrieved lib and dependencies
mtb_deinit: mtb_clean
	$(info )
	$(info Removing mtb_shared and libs folder ...)
	-$(Q) cd $(MTB_LIBS_DIR); rm -rf bsps
	-$(Q) cd $(MTB_LIBS_DIR); rm -rf ../mtb_shared
	-$(Q) cd $(MTB_LIBS_DIR); find . -type f -name assetlocks.json -exec rm -f {} +
	-$(Q) cd $(MTB_LIBS_DIR); find . -type f -name .mtbqueryapi -exec rm -f {} +
	-$(Q) cd $(MTB_LIBS_DIR); find . -type f -name .ninja_log -exec rm -f {} +
	-$(Q) cd $(MTB_LIBS_DIR); find . -type d -name libs -exec rm -rf {} +
#TODO: Check if this file is always created
	-$(Q) cd $(MTB_LIBS_DIR); rm -f proj_cm33_s/nsc_veneer.o 
# TODO: In future we might also need to remove the deps/*.mtb files as in PSOC6
# projects. I keep it here as a reminder.
# 	-$(Q) cd $(MTB_LIBS_DIR); find deps/*.mtb -maxdepth 1 -type f -delete 

mtb_clean:
	$(info )
	$(info Cleaning MTN build projects)
	-$(Q) $(MAKE) -C $(MTB_LIBS_DIR) clean
	-$(Q) cd $(MTB_LIBS_DIR); rm -rf build

# TODO: We don´t want to MTB init every time we build, only once. 
# For now, we will let the user explicitly call mtb_init.
# We will check later how to implement this effectively keeping the 
# required micropython flow:
#
# make submodules
# make BOARD=KIT_PSE84_AI
# make BOARD=KIT_PSE84_AI deploy
#
# Ideally, also we don´t need to specify the boards for every make command
# after the first one.

mtb_build:
	$(info )
	$(info Building $(BOARD) in $(CONFIG) mode using MTB ...)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) build 

.PHONY: mtb_init mtb_deinit mtb_build