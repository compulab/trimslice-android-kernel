#ifndef _TEGRA_DC_RES_H_
#define _TEGRA_DC_RES_H_

#define TEGRA_DC_RES "\
#!/bin/sh\n\
\n\
run_func() {\n\
file=$1\n\
disp=$2\n\
if [ -f $file ]\n\
then\n\
\tcat $file | xargs -l cvt | awk -v out=$disp '/Modeline/ { $1=\"xrandr --newmode\" ; print $0\" ; xrandr --addmode \"out\" \"$2}' | sh -x\n\
fi\n\
}\n\
\n\
COMMAND=\"$1\"\n\
GDM_DEFAULT=\"/etc/gdm/Init/Default\"\n\
TEGRA_DC_RES=`basename $0`\n\
\n\
[ -f ${GDM_DEFAULT} ] || exit 2\n\
\n\
case $COMMAND in\n\
+)\n\
\tgrep ${TEGRA_DC_RES} ${GDM_DEFAULT} > /dev/null\n\
\t[ $? -eq 0 ] && exit 1\n\
\tcp ${GDM_DEFAULT} ${GDM_DEFAULT}.old\n\
\tawk ' { if ($0~/initctl/) { $0=\"/sys/kernel/debug/tegra_dc_res x\\n\"$0  } print }' ${GDM_DEFAULT}.old > ${GDM_DEFAULT}\n\
\t;;\n\
-)\n\
\tgrep ${TEGRA_DC_RES} ${GDM_DEFAULT} > /dev/null\n\
\t[ $? -eq 1 ] && exit 1\n\
\tcp ${GDM_DEFAULT} ${GDM_DEFAULT}.old\n\
\tawk '{ if ($0~/tegra_dc_res/) { next } print  }' ${GDM_DEFAULT}.old > ${GDM_DEFAULT}\n\
\t;;\n\
x)\n\
\trun_func /sys/kernel/debug/tegra_dc_rgb_res LVDS-1\n\
\trun_func /sys/kernel/debug/tegra_dc_hdmi_res HDMI-1\n\
\t;;\n\
*)\n\
\techo \"Usage \"$0\" +/-/x\"\n\
\texit 1\n\
\t;;\n\
esac\n\
"

#endif
