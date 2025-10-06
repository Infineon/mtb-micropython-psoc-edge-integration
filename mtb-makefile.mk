MTB_LIBS_DIR ?= ../../lib/mtb-psoc-edge-libs
MTB_MAKEFILE := $(MTB_LIBS_DIR)/Makefile

ifeq ($(CONFIG),)
CONFIG = $(shell egrep '^ *CONFIG' $(MTB_MAKEFILE) | sed 's/^.*= *//g')
$(info Using CONFIG from environment: $(CONFIG))
else
endif

MTB_LIBS_BUILD_DIR := $(MTB_LIBS_DIR)/build
MTB_LIBS_BUILD_PRJ_HEX_DIR := $(MTB_LIBS_BUILD_DIR)/project_hex

MTB_CM33_NS_BUILD_DIR        := $(MTB_LIBS_DIR)/proj_cm33_ns/build
MTB_CM33_NS_BOARD_BUILD_DIR  := $(MTB_CM33_NS_BUILD_DIR)/APP_$(BOARD)/$(CONFIG)

MTB_NS_STATIC_LIB = $(MTB_CM33_NS_BOARD_BUILD_DIR)/proj_cm33_ns.a

MPY_MTB_MAKE_VARS = BOARD=$(BOARD) CONFIG=$(CONFIG)

mtb_build_ns: $(MTB_NS_STATIC_LIB) 
$(MTB_NS_STATIC_LIB):
	$(info )
	$(info Building $(BOARD) in $(CONFIG) mode using MTB ...)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) $(MPY_MTB_MAKE_VARS) MTB_PROJECTS=proj_cm33_ns build NINJA= CY_IGNORE=main.c

mtb_build_ns_ninja:
	$(info )
	$(info Building $(BOARD) in $(CONFIG) mode using MTB ...)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) $(MPY_MTB_MAKE_VARS) MTB_PROJECTS=proj_cm33_ns build APPNAME=mpy_edge_app_ns LIBNAME= COMBINE_SIGN_JSON=

mtb_build_s:
	$(info )
	$(info Building $(BOARD) in $(CONFIG) mode using MTB ...)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) $(MPY_MTB_MAKE_VARS) MTB_PROJECTS=proj_cm33_s build

mtb_program:
	$(info )
	$(info Deploying firmware in board $(BOARD)...)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) qprogram MTB_PROBE_SERIAL=$(DEVICE_SN) MTB_PROJECTS="proj_cm33_s proj_cm33_ns" $(MPY_MTB_MAKE_VARS) NINJA= 

# TODO: Complete the proper retrieval of building flags.
# TODO: We need to retrieve this from the MTB build system. Hardcoded for now.
LDFLAGS += -mcpu=cortex-m33 --specs=nano.specs -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -mthumb -ffunction-sections -fdata-sections -ffat-lto-objects -g -Wall -pipe -Wl,--gc-sections -Wl,--print-memory-usage -Xlinker -L ../../lib/mtb-psoc-edge-libs/bsps/TARGET_APP_KIT_PSE84_AI/COMPONENT_CM33/TOOLCHAIN_GCC_ARM -Xlinker -L ../../lib/mtb-psoc-edge-libs/bsps/TARGET_APP_KIT_PSE84_AI/config/GeneratedSource -T../../lib/mtb-psoc-edge-libs/bsps/TARGET_APP_KIT_PSE84_AI/COMPONENT_CM33/TOOLCHAIN_GCC_ARM/pse84_ns_cm33.ld -Wl,-Map,/home/enriquezgarc/micropython-psoc-edge/lib/mtb-psoc-edge-libs/proj_cm33_ns/build/APP_KIT_PSE84_AI/Debug/proj_cm33_ns.map -Wl,--start-group ../../lib/mtb-psoc-edge-libs/proj_cm33_s/nsc_veneer.o -Wl,--end-group

mtb_get_build_flags_ns: $(MTB_NS_STATIC_LIB)
	@:
	$(info)
	$(eval MPY_MTB_INCLUDE_DIRS = $(file < $(MTB_LIBS_DIR)/proj_cm33_ns/inclist.rsp))
	$(eval INC += $(subst -I,-I$(MTB_LIBS_DIR)/,$(MPY_MTB_INCLUDE_DIRS)))
# 	$(eval MPY_MTB_LIBRARIES = $(file < $(MTB_CM33_NS_BOARD_BUILD_DIR )/liblist.rsp))
	$(eval LIBS += $(MTB_NS_STATIC_LIB))
	$(eval CFLAGS += $(file < $(MTB_LIBS_DIR)/proj_cm33_ns/.cflags))
# 	$(eval CXXFLAGS += $(file < $(MTB_LIBS_DIR)/proj_cm33_ns/.cxxflags))
# 	$(eval LDFLAGS += $(file < $(MTB_LIBS_DIR)/proj_cm33_ns/.ldflags))
# 	$(eval QSTR_GEN_CFLAGS += $(INC) $(CFLAGS))

mtb_clean:
	$(info )
	$(info Cleaning MTB build projects)
	-$(Q) $(MAKE) -C $(MTB_LIBS_DIR) clean MTB_PROJECTS="proj_cm33_s proj_cm33_ns" $(MPY_MTB_MAKE_VARS)
	-$(Q) rm -rf $(MTB_LIBS_DIR)/build

mtb_build_help:
	@:
	$(info )
	$(info ModusToolbox build available targets:)
	$(info )
	$(info 	mtb_build_s             Build the cm33 secure project)
	$(info 	mtb_build_ns            Build the cm33 non-secure project)
	$(info  mtb_get_build_flags_ns  Retrieve build flags for cm33 non-secure build)
	$(info 	mtb_program             Program the built firmware to the connected board.)
	$(info 	..                      Use DEVICE_SN to specify the board serial number)
	$(info 	mtb_clean               Clean the ModusToolbox build files)
	$(info 	mtb_build_help          Show this help message)
	$(info )

.PHONY: mtb_build_ns mtb_build_s mtb_get_build_flags mtb_program mtb_clean mtb_build_help
