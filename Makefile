all: ext2_cp ext2_mkdir ext2_ln ext2_rm ext2_restore ext2_checker

ext2_cp: 
		gcc -Wall ext2_cp.c -o cp

ext2_mkdir: 
		gcc -Wall ext2_mkdir.c -o mkdir

ext2_ln: 
		gcc -Wall ext2_ln.c -o ln

ext2_rm: 
		gcc -Wall ext2_rm.c -o rm

ext2_restore: 
		gcc -Wall ext2_restore.c -o restore

ext2_checker: 
		gcc -Wall ext2_checker.c -o checker

clean:
		rm ext2_cp ext2_mkdir ext2_ln ext2_rm ext2_restore ext2_checker 
