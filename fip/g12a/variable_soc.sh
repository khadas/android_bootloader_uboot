#!/bin/bash

# static
declare -a BLX_NAME=("bl2" "bl30" "bl31" "bl32" "bl40")

declare -a BLX_SRC_FOLDER=("bl2/src" "bl30/src" "bl31/bl31_1.3/src" "bl32/bl32_2.4/src" "NULL" "bl33")
declare -a BLX_BIN_FOLDER=("bl2/bin" "bl30/bin" "bl31/bl31_1.3/bin" "bl32/bl32_2.4/bin" "NULL")
declare -a BLX_BIN_NAME=("bl2.bin" "bl30.bin" "bl31.bin" "bl32.bin")
declare -a BLX_BIN_NAME_IRDETO=("bl2.irdeto.bin" "bl30.irdeto.bin" "bl31.irdeto.bin" "bl32.bin")
declare -a BLX_BIN_NAME_VMX=("bl2.vmx.bin" "bl30.vmx.bin" "bl31.vmx.bin" "bl32.bin")
declare -a BLX_IMG_NAME=("NULL" "NULL" "bl31.img" "bl32.img")
declare -a BLX_IMG_NAME_IRDETO=("NULL" "NULL" "bl31.irdeto.img" "bl32.img")
declare -a BLX_IMG_NAME_VMX=("NULL" "NULL" "bl31.vmx.img" "bl32.img")
declare -a BLX_NEEDFUL=("true" "true" "true" "false")

declare -a BLX_SRC_GIT=("bootloader/spl" "firmware/scp" "ARM-software/arm-trusted-firmware" "OP-TEE/optee_os" "NULL" "uboot")
declare -a BLX_BIN_GIT=("firmware/bin/bl2" "firmware/bin/bl30" "firmware/bin/bl31" "firmware/bin/bl32" "NULL")

# blx priority. null: default, source: src code, others: bin path
declare -a BIN_PATH=("null" "null" "null" "null" "null")

# variables
declare -a CUR_REV # current version of each blx
declare -a BLX_READY=("false", "false", "false", "false") # blx build/get flag

# package variables
declare BL33_COMPRESS_FLAG=""
declare BL3X_SUFFIX="bin"
declare V3_PROCESS_FLAG=""
declare AML_BL2_NAME=""
declare AML_KEY_BLOB_NAME=""
declare FIP_BL32_PROCESS=""

BUILD_PATH=${FIP_BUILD_FOLDER}

CONFIG_DDR_FW=0
DDR_FW_NAME="aml_ddr.fw"

CONFIG_DDR_PARSE=1
