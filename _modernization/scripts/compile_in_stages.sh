
DEFS="-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE"

STD_ARRAY=(
    "c89"
    "c99"
    "c2x"
)

ARG_ARRAY=(
    ""
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Wformat"
    "-Wformat=2"
    "-Wconversion"
    "-Wsign-conversion"
    "-Wimplicit-fallthrough"
    "-fsanitize=address -fsanitize=undefined -fanalyzer"
    "-Wstrict-prototypes"
    "-Wold-style-declaration"
    "-Wtraditional"
    "-Wtraditional-conversion"
)

# Homebrew version
if [[ -f /opt/homebrew/bin/gcc-15 ]]; then
    GCC="/opt/homebrew/bin/gcc-15"
elif [[ -f /usr/bin/gcc ]]; then
    GCC="/usr/bin/gcc"
else
    echo "NO GCC FOUND"
    exit
fi

GCC_BASE="-O2 -g "

count=0
for std in "${STD_ARRAY[@]}"; do
    for args in "${ARG_ARRAY[@]}"; do
	GCC_ARGS="${GCC_ARGS} ${args} "
	# Use printf to format the count with a leading zero
	PADDED_COUNT=$(printf "%02d" "$count")

	# Use the new padded variable for the filename
	OFILE="/tmp/gcc_errors_${std}_${PADDED_COUNT}"

	/bin/rm -f ${OFILE}
	touch ${OFILE}
	for file in *.c; do
	    ${GCC} ${GCC_BASE} ${GCC_ARGS} -std=${std} ${DEFS} -c ${file} -o /tmp/foo.o >> ${OFILE} 2>&1
	done
	count=$((count + 1))
    done
done
