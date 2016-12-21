
#!/bin/bash

#
# File: ctags.sh
# Author:Zhang Hao 
# Date:2016-11-12 
# Desc: ctags 生成系统库以及ACE, SQLITE, ORACLE相关函数的tags, 存放于 ~/.vim/ctags_encodedemo 文件中
#

mkdir -p ~/.vim;

ctags -R -I __THROW -I __attribute_pure__ -I __nonnull -I __attribute__ --file-scope=yes --langmap=c:+.h --languages=c,c++ --links=yes --c-kinds=+p --c++-kinds=+p --fields=+iaS --extra=+q -f ~/.vim/ctags_encodedemo /usr/include/* /usr/include/sys/* /usr/include/bits/*  /usr/include/netinet/* /usr/include/arpa/* /opt/arm-2009q1-203/arm-none-linux-gnueabi/libc/usr/include/* /usr/local/dvsdk/framework-components_2_26_00_01/packages/ /usr/local/dvsdk/codec-engine_2_26_02_11/packages/ /usr/local/dvsdk/dmai_2_20_00_15/packages/ /usr/local/dvsdk/linuxutils_2_26_01_02/packages/ /usr/local/dvsdk/xdais_6_26_01_03/packages/ /usr/local/dvsdk/xdctools_3_16_03_36/packages/ /usr/local/dvsdk/dvsdk-demos_4_02_00_01/dm365/*

#everytime run this shell, a line would add to the end of .vimrc.so need to delete this line mannually.
echo "set tags+=~/.vim/ctags_encodedemo" >> ~/.vimrc

