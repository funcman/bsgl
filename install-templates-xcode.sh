#!/bin/bash

echo 'bsgl template installer'

BSGL_VER='0.10'
BASE_TEMPLATE_DIR="/Library/Application Support/Developer/Shared/Xcode"
BASE_TEMPLATE_USER_DIR="$HOME/Library/Application Support/Developer/Shared/Xcode"

force=
user_dir=

usage(){
cat << EOF
usage: $0 [options]

Install / update templates for ${COCOS2D_VER}

OPTIONS:
   -f	force overwrite if directories exist
   -h	this help
   -u	install in user's Library directory instead of global directory
EOF
}

while getopts "fhu" OPTION; do
	case "$OPTION" in
		f)
			force=1
			;;
		h)
			usage
			exit 0
			;;
		u)
			user_dir=1
			;;
	esac
done

# Make sure only root can run our script
if [[ ! $user_dir  && "$(id -u)" != "0" ]]; then
	echo ""
	echo "Error: This script must be run as root in order to copy templates to ${BASE_TEMPLATE_DIR}" 1>&2
	echo ""
	echo "Try running it with 'sudo', or with '-u' to install it only you:" 1>&2
	echo "   sudo $0" 1>&2
	echo "or:" 1>&2
	echo "   $0 -u" 1>&2
exit 1
fi

# Make sure root and user_dir is not executed at the same time
if [[ $user_dir && "$(id -u)" == "0" ]]; then
	echo ""
	echo "Error: Do not run this script as root with the '-u' option." 1>&2
	echo ""
	echo "Either use the '-u' option or run it as root, but not both options at the same time." 1>&2
	echo ""
	echo "RECOMMENDED WAY:" 1>&2
	echo " $0 -u -f" 1>&2
	echo ""
exit 1
fi

copy_files(){
    # SRC_DIR="${SCRIPT_DIR}/${1}"
	#rsync -r --exclude=.svn "$1" "$2"
	rsync -r "$1" "$2"
}

check_dst_dir(){
	if [[ -d $DST_DIR ]];  then
		if [[ $force ]]; then
			echo "removing old libraries: ${DST_DIR}"
			rm -rf "${DST_DIR}"
		else
			echo "templates already installed. To force a re-install use the '-f' parameter"
			exit 1
		fi
	fi

	echo ...creating destination directory: $DST_DIR
	mkdir -p "$DST_DIR"
}

copy_bsgl_files(){
	echo ...copying bsgl files
	copy_files include "$LIBS_DIR"/bsgl
	copy_files src/core "$LIBS_DIR"/bsgl
	copy_files src/advance "$LIBS_DIR"/bsgl
}

copy_tinyxml_files(){
	echo ...copying tinyxml files
	copy_files src/dependencies/tinyxml "$LIBS_DIR"
}

print_template_banner(){
	echo ''
	echo ''
	echo ''
	echo "$1"
	echo '----------------------------------------------------'
	echo ''
}

# Xcode4 templates
copy_xcode4_project_templates(){
    TEMPLATE_DIR="$HOME/Library/Developer/Xcode/Templates/bsgl/"

	print_template_banner "Installing Xcode 4 bsgl iOS template"

	DST_DIR="$TEMPLATE_DIR"
    check_dst_dir

	LIBS_DIR="$DST_DIR""lib_bsgl.xctemplate/libs/"
    mkdir -p "$LIBS_DIR"
    copy_bsgl_files

	LIBS_DIR="$DST_DIR""lib_tinyxml.xctemplate/libs/"
    mkdir -p "$LIBS_DIR"
    copy_tinyxml_files


	echo ...copying template files
	copy_files template/xcode4/ "$DST_DIR"

	echo done!

    # Move File Templates to correct position
	# DST_DIR="$HOME/Library/Developer/Xcode/Templates/File Templates/bsgl/"
	# OLD_DIR="$HOME/Library/Developer/Xcode/Templates/bsgl/"

	# print_template_banner "Installing Xcode 4 CCNode file templates..."

    # check_dst_dir

	# mv -f "$OLD_DIR""/CCNode class.xctemplate" "$DST_DIR"

	echo done!

}

copy_xcode4_project_templates

