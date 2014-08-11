#!/bin/bash

#------------IMPORTANT------------#
#--RUN THIS SCRIPT BEFOR COMMIT---#
#---------------------------------#

#usage:
#

if [ "${CROSS_COMPILE}" ] ; then
	MAKE="make CROSS_COMPILE=${CROSS_COMPILE}"
else
	MAKE=make
fi



build_target() {

    if [ "$1" == "m6v1" ]; then
        target=m6_mbx_v1
        out=out_m6v1
    elif [ "$1" == "m6v2" ]; then
        target=m6_mbx_v2
        out=out_m6v2
    elif [ "$1" == "m8" ]; then
        target=m8_k200_v1
        out=out_k200
    fi

	LOG_DIR=${out}/LOG

	${MAKE} distclean >/dev/null
	${MAKE} O=${out} ${target}_config


    [ -d ${LOG_DIR} ] || mkdir ${LOG_DIR} || exit 1

	${MAKE} O=${out} -j8 2>&1 >${LOG_DIR}/$target.MAKELOG \
				| tee ${LOG_DIR}/$target.ERR

	# Check for 'make' errors
	if [ ${PIPESTATUS[0]} -ne 0 ] ; then
		RC=1
	fi

	if [ -s ${LOG_DIR}/$target.ERR ] ; then
		ERR_CNT=$((ERR_CNT + 1))
		ERR_LIST="${ERR_LIST} $target"
	else
		rm ${LOG_DIR}/$target.ERR
	fi


	${CROSS_COMPILE}size ${out}/u-boot \
				| tee -a ${LOG_DIR}/$target.MAKELOG
}



build_target "$@"
