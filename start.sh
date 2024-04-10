#!/usr/bin/env bash
_term() {
  echo "$(date --utc +%FT%T.%10N): Caught SIGTERM signal"
  kill -TERM "$ME_PID"
  echo "$(date --utc +%FT%T.%10N): sent TERM signal to PID: $ME_PID"
}

trap _term SIGTERM

SCRIPT=`realpath $0`
export APPDIR=`dirname $SCRIPT`
echo "$(date --utc +%FT%T.%10N): Pulsar host is : ${PLSR_URL}"

PAIR=$1
ME_BIN=matching_engine

BASEDIR=${APPDIR}
export CORE_LOCATION=${APPDIR}
export DT=$(date +%Y%m%d_%H%M%S)
export LOG_LOCATION=${APPDIR}/log/$DT
echo ${LOG_LOCATION} > ${APPDIR}/LOG_LOCATION
export LD_LIBRARY_PATH=${BASEDIR}/usr/local/lib64:${BASEDIR}/usr/local/lib:${BASEDIR}/usr/local:$LD_LIBRARY_PATH

mkdir -p ${LOG_LOCATION}
cd ${APPDIR}/log
ln -snf ${DT} clog
cd ${APPDIR}

cat $APPDIR/logrotate.cron >> /etc/crontab
crond
crontab /etc/crontab

#$APPDIR/${ME_BIN} ${PLSR_URL} $PAIR 1 >> ${LOG_LOCATION}/me_server.out.log 2>> ${LOG_LOCATION}/me_server.err.log &
$APPDIR/${ME_BIN} ${PLSR_URL} $PAIR 2>&1 | tee -a ${LOG_LOCATION}/${PAIR}_me_server.log &
echo "$(date --utc +%FT%T.%10N): $PAIR matching engine process started!"

ME_PID=$!
wait

echo "$(date --utc +%FT%T.%10N): finished wait, tailing ME PID ($ME_PID)"
tail --pid=$ME_PID -f /dev/null

echo "$(date --utc +%FT%T.%10N): ME PID ($ME_PID) have been terminated gracefully"
