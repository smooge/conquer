#!/bin/bash

# Subphase 3: Test all files with -Wall flag
# This script tests compilation with -Wall to establish warning baseline

#OUTFILE=_modernization/claude/reports/PHASE_4.3_WALL_BASELINE.txt
OUTFILE=_modernization/claude/reports/PHASE_4.3_WALL_CURRENT.txt
STD="c99"
WARN="-Wall"
TEMPFILE=$(mktemp /tmp/my-app-data.XXXXXX)

# Define compilation flags for each file type
ADMIN_FLAGS="-O2 -g -std=${STD} -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -DDEFAULTDIR=\"/home/ssmoogen/conquer/lib\" -DEXEDIR=\"/home/ssmoogen/conquer/bin\" -DLOGIN=\"ssmoogen\" -DADMIN -DCONQUER ${WARN}"

GAME_FLAGS="-O2 -g -std=${STD} -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -DDEFAULTDIR=\"/home/ssmoogen/conquer/lib\" -DEXEDIR=\"/home/ssmoogen/conquer/bin\"  -DLOGIN=\"ssmoogen\" -DCONQUER ${WARN}"

PSMAP_FLAGS="-O2 -g -std=${STD} -O2 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -DDEFAULTDIR=\"/home/ssmoogen/conquer/lib\" -DEXEDIR=\"/home/ssmoogen/conquer/bin\" -DLOGIN=\"ssmoogen\" -DPSFILE=\"/home/ssmoogen/conquer/bin/psmap.ps\" -DLETTER ${WARN}"


echo "=== SUBPHASE 3: Testing All Files with -Wall Flag ===" > ${OUTFILE}
echo "Date: $(date)" >> ${OUTFILE}
echo "" >> ${OUTFILE}


# zero out the file
cat /dev/null > ${OUTFILE}


echo "=== ADMIN-ONLY FILES (8 files) ===" >> ${OUTFILE}
for file in admin.c combat.c spew.c newlogin.c update.c npc.c randeven.c makeworl.c; do
    echo "--- Testing $file ---" >> ${OUTFILE}
    gcc $ADMIN_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
    cat ${TEMPFILE} >> ${OUTFILE}
    if grep -q "error:" ${TEMPFILE}; then
        echo "RESULT: COMPILATION ERROR" >> ${OUTFILE}
    else
        echo "RESULT: SUCCESS (warnings only)" >> ${OUTFILE}
    fi
    echo "" >> ${OUTFILE}
done

echo "=== SHARED FILES (1 file) ===" >> ${OUTFILE}
echo "--- Testing check.c ---" >> ${OUTFILE}
gcc $ADMIN_FLAGS -c "check.c" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
cat ${TEMPFILE} >> ${OUTFILE}

if grep -q error: ${TEMPFILE}; then
    echo "RESULT: SUCCESS (warnings only)" >> ${OUTFILE}
else
    echo "RESULT: COMPILATION ERROR" >> ${OUTFILE}
fi
echo "" >> ${OUTFILE}

echo "=== GAME-ONLY FILES (7 files) ===" >> ${OUTFILE}
for file in commands.c main.c forms.c move.c reports.c display.c extcmds.c; do
    echo "--- Testing $file ---" >> ${OUTFILE}
    gcc $GAME_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
    cat ${TEMPFILE} >> ${OUTFILE}
    if grep -q "error:" ${TEMPFILE}; then
        echo "RESULT: COMPILATION ERROR" >> ${OUTFILE}
    else
        echo "RESULT: SUCCESS (warnings only)" >> ${OUTFILE}
    fi
    echo "" >> ${OUTFILE}
done

echo "=== DUAL-COMPILED FILES (7 files) ===" >> ${OUTFILE}
for file in cexecute.c io.c misc.c navy.c magic.c data.c trade.c; do
    echo "--- Testing $file (Admin mode) ---" >> ${OUTFILE}
    gcc $ADMIN_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
    cat ${TEMPFILE} >> ${OUTFILE}
    if grep -q "error:" ${TEMPFILE}; then
        echo "ADMIN RESULT: COMPILATION ERROR" >> ${OUTFILE}
    else
        echo "ADMIN RESULT: SUCCESS (warnings only)" >> ${OUTFILE}
    fi

    echo "--- Testing $file (Game mode) ---" >> ${OUTFILE}
    gcc $GAME_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
    cat ${TEMPFILE} >> ${OUTFILE}
    if grep -q "error:" ${TEMPFILE}; then
        echo "GAME RESULT: COMPILATION ERROR" >> ${OUTFILE}
    else
        echo "GAME RESULT: SUCCESS (warnings only)" >> ${OUTFILE}
    fi
    echo "" >> ${OUTFILE}
done

echo "=== POSTSCRIPT UTILITY FILES (1 file) ===" >> ${OUTFILE}
echo "--- Testing psmap.c ---" >> ${OUTFILE}
gcc $PSMAP_FLAGS -c "psmap.c" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
cat ${TEMPFILE} >> ${OUTFILE}
if grep -q error: ${TEMPFILE}; then
    echo "RESULT: SUCCESS (warnings only)" >> ${OUTFILE}
else
    echo "RESULT: COMPILATION ERROR" >> ${OUTFILE}
fi
echo "" >> ${OUTFILE}

echo "=== SUBPHASE 3 TESTING COMPLETE ===" >> ${OUTFILE}
echo "Check report for warning counts and compilation status." >> ${OUTFILE}

rm -f ${TEMPFILE}
