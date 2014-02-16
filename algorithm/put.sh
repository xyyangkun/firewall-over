#!/bin/sh
echo $PWD
psftp -b ./sftp_cmds_client.txt xyyangkun@1.linuxlearn.net
psftp -b ./sftp_cmds_server.txt xy@163.yangkuncn.cn
