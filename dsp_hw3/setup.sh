#!/bin/bash +v

export SRILM_REP_PATH="/opt/kaldi/tools/srilm"
export MACHINE_TYPE="i686-m64"

if [ ! -v SRILM_BIN_PATH ]
then
        export SRILM_BIN_PATH="$SRILM_REP_PATH/bin/$MACHINE_TYPE"
        export PATH="$SRILM_BIN_PATH:$PATH"
        export PS1="(srilm) $PS1"

        # makefile
        export SRIPATH=$SRILM_REP_PATH
fi

deactivate() {
        export PATH=${PATH#$(echo $SRILM_BIN_PATH):}
        export PS1=${PS1#(srilm) }
        unset SRILM_REP_PATH
        unset SRILM_BIN_PATH
        unset MACHINE_TYPE
        unset -f deactivate
}
export -f deactivate
