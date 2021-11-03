//
// Operating System
// Simple FIle System
// make by Park Dongjun
// gcc sfs_disk.c sfs_func_my.c sfs_main.c sfs_func_ext.o
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* optional */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/***********/

#include "sfs_types.h"
#include "sfs_func.h"
#include "sfs_disk.h"
#include "sfs.h"

void dump_directory();

/* BIT operation Macros */
/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

static struct sfs_super spb;	// superblock
static struct sfs_dir sd_cwd = { SFS_NOINO }; // current working directory

void error_message(const char *message, const char *path, int error_code) {
	switch (error_code) {
	case -1:
		printf("%s: %s: No such file or directory\n",message, path); return;
	case -2:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -3:
		printf("%s: %s: Directory full\n",message, path); return;
	case -4:
		printf("%s: %s: No block available\n",message, path); return;
	case -5:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -6:
		printf("%s: %s: Already exists\n",message, path); return;
	case -7:
		printf("%s: %s: Directory not empty\n",message, path); return;
	case -8:
		printf("%s: %s: Invalid argument\n",message, path); return;
	case -9:
		printf("%s: %s: Is a directory\n",message, path); return;
	case -10:
		printf("%s: %s: Is not a file\n",message, path); return;
	default:
		printf("unknown error code\n");
		return;
	}
}

void sfs_mount(const char* path)
{
	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}

	printf("Disk image: %s\n", path);

	disk_open(path);
	disk_read( &spb, SFS_SB_LOCATION );

	printf("Superblock magic: %x\n", spb.sp_magic);

	assert( spb.sp_magic == SFS_MAGIC );
	
	printf("Number of blocks: %d\n", spb.sp_nblocks);
	printf("Volume name: %s\n", spb.sp_volname);
	printf("%s, mounted\n", spb.sp_volname);
	
	sd_cwd.sfd_ino = 1;		//init at root
	sd_cwd.sfd_name[0] = '/';
	sd_cwd.sfd_name[1] = '\0';
}

void sfs_umount() {

	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}
}

void sfs_touch(const char* path)
{	
	int i, j;
	struct sfs_inode si;
	disk_read(&si, sd_cwd.sfd_ino);
	
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	//check path
	for(i = 0; i < SFS_NDIRECT; i++){
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino > 0){	
				if(!strcmp(path, sd[j].sfd_name)){
					error_message("touch", path, -6);
					return;
				}
			}
		}
	}

	//check dir entry	
	int neede = 0;
	int dirEntry[2];
	int emptyDir;
	int scheck = 0;
	int kcheck = 0;

	for(i = 0; i < SFS_NDIRECT; i++){
		if(scheck == 1){
			break;
		}
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino <= 0){	
				kcheck = 1;
				scheck = 1;
				dirEntry[0] = i;
				dirEntry[1] = j;
				break;
			}
		}
	}
	if(scheck == 0){
		for(i = 0; i < SFS_NDIRECT; i++){
			if(si.sfi_direct[i] <= 0){
				kcheck = 1;
				emptyDir = i;
				neede = 1;
				break;
			}
		}
	}
	if(kcheck == 0){
		error_message("touch", path, -3);
		return;
	}

	//check bitmap
	int pcheck = 0;
	int emptyi[2];
	int k;
	int nbitblock = spb.sp_nblocks;
	int bit[SFS_BITBLOCKS(nbitblock)][128];
	
	for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
		disk_read(bit[i], (SFS_MAP_LOCATION + i));
	}
	for(k = 0; k < SFS_BITBLOCKS(nbitblock); k++){
		if(pcheck == (1 + neede)){
			break;
		}
		for(i = 0; i < 128; i++){
			if(pcheck == (1 + neede)){
				break;
			}
			int check = bit[k][i];
			for(j = 0; j < 32; j++){
				if(!(BIT_CHECK(check, j))){
					BIT_SET(check, j);
					bit[k][i] = check;
					emptyi[pcheck] = (k * 4096) + (i * 32) + j;
					pcheck++;
					if(pcheck == (1 + neede)){
						break;
					}
				}
			}
		}
	}

	if(pcheck <= (0 + neede)){
		error_message("touch", path, -4);
		return;
	}
	else{
		for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
			disk_write(bit[i], (SFS_MAP_LOCATION + i));
		}
	}

	//neede entry
	if(neede == 1){
		disk_read(&si, sd_cwd.sfd_ino);
		si.sfi_size += sizeof(struct sfs_dir);
		si.sfi_direct[emptyDir] = emptyi[1];
		disk_write(&si, sd_cwd.sfd_ino);

		struct sfs_dir input[SFS_DENTRYPERBLOCK];
		bzero(input, (sizeof(struct sfs_dir) * SFS_DENTRYPERBLOCK));
		input[0].sfd_ino = emptyi[0];
		strcpy(input[0].sfd_name, path);
		disk_write(input, emptyi[1]);
	}  
	else{
		disk_read(&si, sd_cwd.sfd_ino);
		si.sfi_size += sizeof(struct sfs_dir);
		disk_write(&si, sd_cwd.sfd_ino);

		disk_read(sd, si.sfi_direct[dirEntry[0]]);
		sd[dirEntry[1]].sfd_ino = emptyi[0];
		strcpy(sd[dirEntry[1]].sfd_name, path);
		disk_write(sd, si.sfi_direct[dirEntry[0]]);
	}

	//inode setting
	struct sfs_inode newthing;
	bzero(&newthing, sizeof(struct sfs_inode));
	newthing.sfi_size = 0;
	newthing.sfi_type = SFS_TYPE_FILE;
	disk_write(&newthing, emptyi[0]);
}

void sfs_cd(const char* path)
{	
	int i, j;
	int pcheck = 0;
	struct sfs_inode si, check;
	disk_read(&si, sd_cwd.sfd_ino);

	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	if(path == NULL){
		sd_cwd.sfd_ino = SFS_ROOT_LOCATION;
	}
	else if(path != NULL){
		for(i = 0; i < SFS_NDIRECT; i++){
			if(pcheck == 1){
				break;
			}
			if(si.sfi_direct[i] <= 0){
				continue;
			}
			disk_read(sd, si.sfi_direct[i]);
			for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
				if(sd[j].sfd_ino > 0){
					disk_read(&check, sd[j].sfd_ino);
					if(!strcmp(path, sd[j].sfd_name)){
						if(check.sfi_type == SFS_TYPE_FILE){
							error_message("cd", path, -2);
							return;
						}
						sd_cwd.sfd_ino = sd[j].sfd_ino;
						strcpy(sd_cwd.sfd_name, sd[j].sfd_name);
						pcheck = 1;
						break;
					}
				}
			}
		}
		if(pcheck == 0){
			error_message("cd", path, -1);
			return;
		}	
	}
}

void sfs_ls(const char* path)
{
	int i, j;
	int pcheck = 0;
	struct sfs_inode si, check;
	disk_read(&si, sd_cwd.sfd_ino);

	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	if(path != NULL){
		for(i = 0; i < SFS_NDIRECT; i++){
			if(pcheck == 1){
				break;
			}
			if(si.sfi_direct[i] <= 0){
				continue;
			}
			disk_read(sd, si.sfi_direct[i]);
			for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
				if(sd[j].sfd_ino > 0){
					disk_read(&check, sd[j].sfd_ino);
					if(!strcmp(path, sd[j].sfd_name)){
						if(check.sfi_type == SFS_TYPE_FILE){
							printf("%s\n", path);
							return;
						}
						disk_read(&si, sd[j].sfd_ino);
						pcheck = 1;
						break;
					}
				}
			}
		}
		if(pcheck == 0){
			error_message("ls", path, -1);
			return;
		}	
	}

	for(i = 0; i < SFS_NDIRECT; i++){
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino > 0){
				disk_read(&check, sd[j].sfd_ino);
				printf("%s", sd[j].sfd_name);
				if(check.sfi_type == SFS_TYPE_DIR){
					printf("/");
				}
				printf("\t");
			}
		}
	}
	printf("\n");
}

void sfs_mkdir(const char* org_path) 
{
	int i, j;
	struct sfs_inode si;
	disk_read(&si, sd_cwd.sfd_ino);
	
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	//check org path
	for(i = 0; i < SFS_NDIRECT; i++){
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino > 0){	
				if(!strcmp(org_path, sd[j].sfd_name)){
					error_message("mkdir", org_path, -6);
					return;
				}
			}
		}
	}

	//check dir entry	
	int neede = 0;
	int dirEntry[2];
	int emptyDir;
	int scheck = 0;
	int kcheck = 0;

	for(i = 0; i < SFS_NDIRECT; i++){
		if(scheck == 1){
			break;
		}
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino <= 0){	
				kcheck = 1;
				scheck = 1;
				dirEntry[0] = i;
				dirEntry[1] = j;
				break;
			}
		}
	}
	if(scheck == 0){
		for(i = 0; i < SFS_NDIRECT; i++){
			if(si.sfi_direct[i] <= 0){
				kcheck = 1;
				emptyDir = i;
				neede = 1;
				break;
			}
		}
	}
	if(kcheck == 0){
		error_message("mkdir", org_path, -3);
		return;
	}

	//check bitmap
	int pcheck = 0;
	int emptyi[3];
	int k;
	int nbitblock = spb.sp_nblocks;
	int bit[SFS_BITBLOCKS(nbitblock)][128];
	
	for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
		disk_read(bit[i], (SFS_MAP_LOCATION + i));
	}
	for(k = 0; k < SFS_BITBLOCKS(nbitblock); k++){
		if(pcheck == (2 + neede)){
			break;
		}
		for(i = 0; i < 128; i++){
			if(pcheck == (2 + neede)){
				break;
			}
			int check = bit[k][i];
			for(j = 0; j < 32; j++){
				if(!(BIT_CHECK(check, j))){
					BIT_SET(check, j);
					bit[k][i] = check;
					emptyi[pcheck] = (k * 4096) + (i * 32) + j;
					pcheck++;
					if(pcheck == (2 + neede)){
						break;
					}
				}
			}
		}
	}

	if(pcheck <= (1 + neede)){
		error_message("mkdir", org_path, -4);
		return;
	}
	else{
		for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
			disk_write(bit[i], (SFS_MAP_LOCATION + i));
		}
	}

	//neede entry
	if(neede == 1){
		//order change
		int temp = emptyi[0];
		emptyi[0] = emptyi[1];
		emptyi[1] = emptyi[2];
		emptyi[2] = temp;

		disk_read(&si, sd_cwd.sfd_ino);
		si.sfi_size += sizeof(struct sfs_dir);
		si.sfi_direct[emptyDir] = emptyi[2];
		disk_write(&si, sd_cwd.sfd_ino);

		struct sfs_dir input[SFS_DENTRYPERBLOCK];
		bzero(input, (sizeof(struct sfs_dir) * SFS_DENTRYPERBLOCK));
		input[0].sfd_ino = emptyi[0];
		strcpy(input[0].sfd_name, org_path);
		disk_write(input, emptyi[2]);
	}  
	else{
		disk_read(&si, sd_cwd.sfd_ino);
		si.sfi_size += sizeof(struct sfs_dir);
		disk_write(&si, sd_cwd.sfd_ino);

		disk_read(sd, si.sfi_direct[dirEntry[0]]);
		sd[dirEntry[1]].sfd_ino = emptyi[0];
		strcpy(sd[dirEntry[1]].sfd_name, org_path);
		disk_write(sd, si.sfi_direct[dirEntry[0]]);
	}
	
	//dir, inode setting
	struct sfs_inode newthing;
	bzero(&newthing, sizeof(struct sfs_inode));
	newthing.sfi_size = 128;
	newthing.sfi_type = SFS_TYPE_DIR;
	newthing.sfi_direct[0] = emptyi[1];
	disk_write(&newthing, emptyi[0]);

	struct sfs_dir newdir[SFS_DENTRYPERBLOCK];
	bzero(newdir, (sizeof(struct sfs_dir) * SFS_DENTRYPERBLOCK));
	newdir[0].sfd_ino = emptyi[0];
	strcpy(newdir[0].sfd_name, ".");
	newdir[1].sfd_ino = sd_cwd.sfd_ino;
	strcpy(newdir[1].sfd_name, "..");
	disk_write(newdir, emptyi[1]);
}

void sfs_rmdir(const char* org_path) 
{
	int i, j;
	int pcheck = 0;
	int dirNum;
	int rmdirNum;
	struct sfs_inode si, check;
	disk_read(&si, sd_cwd.sfd_ino);

	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	for(i = 0; i < SFS_NDIRECT; i++){
		if(pcheck == 1){
			break;
		}
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		dirNum = si.sfi_direct[i];
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino > 0){
				disk_read(&check, sd[j].sfd_ino);
				if(!strcmp(org_path, ".")){
					error_message("rmdir", org_path, -8);
					return;
				}
				else if(!strcmp(org_path, sd[j].sfd_name)){
					if(check.sfi_type == SFS_TYPE_FILE){
						error_message("rmdir", org_path, -2);
						return;
					}
					disk_read(&si, sd[j].sfd_ino);
					
					//empty dir check	
					if(si.sfi_size > 128){
						error_message("rmdir", org_path, -7);
						return;
					}
					
					rmdirNum = sd[j].sfd_ino;
					//rm dir entry
					sd[j].sfd_ino = 0;
					disk_write(sd, dirNum);
					pcheck = 1;
					break;
				}
			}
		}
	}
	if(pcheck == 0){
		error_message("rmdir", org_path, -1);
		return;
	}

	//delete bitmap
	int b, t, s;
	int tempBit;	
	int tempI;
	int nbitblock = spb.sp_nblocks;
	int bit[SFS_BITBLOCKS(nbitblock)][128];

	for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
		disk_read(bit[i], (SFS_MAP_LOCATION + i));
	}

	//dir dir
	tempI = si.sfi_direct[0];
	
	b = (tempI / 4096);
	t = ((tempI % 4096)/ 32);
	s = (tempI % 32);
	tempBit = bit[b][t];
	BIT_CLEAR(tempBit, s);
	bit[b][t] = tempBit;

	//dir inode
	tempI = rmdirNum;
	
	b = (tempI / 4096);
	t = ((tempI % 4096)/ 32);
	s = (tempI % 32);
	tempBit = bit[b][t];
	BIT_CLEAR(tempBit, s);
	bit[b][t] = tempBit;

	for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
		disk_write(bit[i], (SFS_MAP_LOCATION + i));
	}
	
	//reduce sfi_size
	disk_read(&si, sd_cwd.sfd_ino);
	si.sfi_size -= sizeof(struct sfs_dir);
	disk_write(&si, sd_cwd.sfd_ino);
}

void sfs_mv(const char* src_name, const char* dst_name) 
{
	int i, j;
	int pcheck = 0;
	struct sfs_inode si;
	disk_read(&si, sd_cwd.sfd_ino);

	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	//check dst_name
	for(i = 0; i < SFS_NDIRECT; i++){
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino > 0){	
				if(!strcmp(dst_name, sd[j].sfd_name)){
					error_message("mv", dst_name, -6);
					return;
				}
			}
		}
	}
	
	//change name	
	for(i = 0; i < SFS_NDIRECT; i++){
		if(pcheck == 1){
			break;
		}
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino > 0){
				if(!strcmp(src_name, sd[j].sfd_name)){
					strcpy(sd[j].sfd_name, dst_name);
					disk_write(sd, si.sfi_direct[i]);
					pcheck = 1;
					break;
				}
			}
		}
	}
	if(pcheck == 0){
		error_message("mv", src_name, -1);
		return;
	}	
}

void sfs_rm(const char* path) 
{
	int i, j;
	int pcheck = 0;
	int rmfileNum;
	int dirNum;
	struct sfs_inode si, check;
	disk_read(&si, sd_cwd.sfd_ino);

	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	for(i = 0; i < SFS_NDIRECT; i++){
		if(pcheck == 1){
			break;
		}
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		dirNum = si.sfi_direct[i];
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino > 0){
				disk_read(&check, sd[j].sfd_ino);
				if(!strcmp(path, sd[j].sfd_name)){
					if(check.sfi_type == SFS_TYPE_DIR){
						error_message("rm", path, -9);
						return;
					}
					disk_read(&si, sd[j].sfd_ino);
					rmfileNum = sd[j].sfd_ino;
					sd[j].sfd_ino = 0;
					disk_write(sd, dirNum);
					pcheck = 1;
					break;
				}
			}
		}
	}
	if(pcheck == 0){
		error_message("rm", path, -1);
		return;
	}

	//delete bitmap
	int b, t, s;
	int tempBit;	
	int tempI;
	int nbitblock = spb.sp_nblocks;
	int bit[SFS_BITBLOCKS(nbitblock)][128];

	for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
		disk_read(bit[i], (SFS_MAP_LOCATION + i));
	}

	for(i = 0; i < SFS_NDIRECT; i++){
		if(si.sfi_direct[i] > 0){
			tempI = si.sfi_direct[i];
			si.sfi_direct[i] = 0;
	
			b = (tempI / 4096);
			t = ((tempI % 4096) / 32);
			s = (tempI % 32);
			tempBit = bit[b][t];
			BIT_CLEAR(tempBit, s);
			bit[b][t] = tempBit;
		}
	}
	
	if(si.sfi_indirect > 0){	
		int dir[128];
		disk_read(dir, si.sfi_indirect);
		for(i = 0; i < 128; i++){
			if(dir[i] > 0){	
				tempI = dir[i];
	
				b = (tempI / 4096);
				t = ((tempI % 4096) / 32);
				s = (tempI % 32);
				tempBit = bit[b][t];
				BIT_CLEAR(tempBit, s);
				bit[b][t] = tempBit;
			}
		}
		tempI = si.sfi_indirect;
		si.sfi_indirect = 0;
	
		b = (tempI / 4096);
		t = ((tempI % 4096) / 32);
		s = (tempI % 32);
		tempBit = bit[b][t];
		BIT_CLEAR(tempBit, s);
		bit[b][t] = tempBit;
	}

	//rm final delete self
	tempI = rmfileNum;
	
	b = (tempI / 4096);
	t = ((tempI % 4096)/ 32);
	s = (tempI % 32);
	tempBit = bit[b][t];
	BIT_CLEAR(tempBit, s);
	bit[b][t] = tempBit;

	for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
		disk_write(bit[i], (SFS_MAP_LOCATION + i));
	}
	
	//reduce sfi_size
	disk_read(&si, sd_cwd.sfd_ino);
	si.sfi_size -= sizeof(struct sfs_dir);
	disk_write(&si, sd_cwd.sfd_ino);
}

void sfs_cpin(const char* local_path, const char* path) 
{
	int i, j;
	int pcheck = 0;
	struct sfs_inode si;
	disk_read(&si, sd_cwd.sfd_ino);

	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	//check dst_name
	for(i = 0; i < SFS_NDIRECT; i++){
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino > 0){	
				if(!strcmp(local_path, sd[j].sfd_name)){
					error_message("cpin", local_path, -6);
					return;
				}
			}
		}
	}
	
	//local path check	
	FILE *infile = fopen(path, "r");
	if(infile == NULL){
		printf("%s: can't open %s input file\n", "cpin", path);
		return;
	}
	
	//size check
	fseek(infile, 0, SEEK_END);
	int size = ftell(infile);
	if(size > 73216){
		printf("%s: input file size exceeds the max file size\n","cpin");
		return;
	}
	fclose(infile);
	infile = fopen(path, "r");

	//check dir entry	
	int neede = 0;
	int dirEntry[2];
	int emptyDir;
	int scheck = 0;
	int kcheck = 0;

	for(i = 0; i < SFS_NDIRECT; i++){
		if(scheck == 1){
			break;
		}
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino <= 0){	
				kcheck = 1;
				scheck = 1;
				dirEntry[0] = i;
				dirEntry[1] = j;
				break;
			}
		}
	}
	if(scheck == 0){
		for(i = 0; i < SFS_NDIRECT; i++){
			if(si.sfi_direct[i] <= 0){
				kcheck = 1;
				emptyDir = i;
				neede = 1;
				break;
			}
		}
	}
	if(kcheck == 0){
		error_message("cpin", local_path, -3);
		return;
	}
	
	//check bitmap
	pcheck = 0;
	int emptyi[2];
	int k;
	int nbitblock = spb.sp_nblocks;
	int bit[SFS_BITBLOCKS(nbitblock)][128];
	
	for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
		disk_read(bit[i], (SFS_MAP_LOCATION + i));
	}
	for(k = 0; k < SFS_BITBLOCKS(nbitblock); k++){
		if(pcheck == (1 + neede)){
			break;
		}
		for(i = 0; i < 128; i++){
			if(pcheck == (1 + neede)){
				break;
			}
			int check = bit[k][i];
			for(j = 0; j < 32; j++){
				if(!(BIT_CHECK(check, j))){
					BIT_SET(check, j);
					bit[k][i] = check;
					emptyi[pcheck] = (k * 4096) + (i * 32) + j;
					pcheck++;
					if(pcheck == (1 + neede)){
						break;
					}
				}
			}
		}
	}

	if(pcheck <= (0 + neede)){
		error_message("cpin", local_path, -4);
		return;
	}
	else{
		for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
			disk_write(bit[i], (SFS_MAP_LOCATION + i));
		}
	}

	//neede entry
	if(neede == 1){
		int temp = emptyi[1];
		emptyi[1] = emptyi[0];
		emptyi[0] = temp;
		disk_read(&si, sd_cwd.sfd_ino);
		si.sfi_size += sizeof(struct sfs_dir);
		si.sfi_direct[emptyDir] = emptyi[1];
		disk_write(&si, sd_cwd.sfd_ino);

		struct sfs_dir input[SFS_DENTRYPERBLOCK];
		bzero(input, (sizeof(struct sfs_dir) * SFS_DENTRYPERBLOCK));
		input[0].sfd_ino = emptyi[0];
		strcpy(input[0].sfd_name, local_path);
		disk_write(input, emptyi[1]);
	}  
	else{
		disk_read(&si, sd_cwd.sfd_ino);
		si.sfi_size += sizeof(struct sfs_dir);
		disk_write(&si, sd_cwd.sfd_ino);

		disk_read(sd, si.sfi_direct[dirEntry[0]]);
		sd[dirEntry[1]].sfd_ino = emptyi[0];
		strcpy(sd[dirEntry[1]].sfd_name, local_path);
		disk_write(sd, si.sfi_direct[dirEntry[0]]);
	}

	//inode setting
	struct sfs_inode newthing;
	bzero(&newthing, sizeof(struct sfs_inode));
	newthing.sfi_size = 0;
	newthing.sfi_type = SFS_TYPE_FILE;
	disk_write(&newthing, emptyi[0]);
	
	//make block
	char block[512];
	int indblock[128];
	int l, y;
	char ctemp[1];
	int emptyinode;
	int incheck = 0;
	int sizecount;
	int bitcheck[3] = {0, 0, 0};
	pcheck = 0;
	
	for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
		disk_read(bit[i], (SFS_MAP_LOCATION + i));
	}

	for(i = 0; i < SFS_NDIRECT; i++){
		if(incheck == 1){
			break;
		}
		bzero(block, (sizeof(char) * 512));
		sizecount = 0;
		for(j = 0; j < 128; j++){
			if(incheck == 1){
				break;
			}
			for(k = 0; k < 4; k++){
				if(!fgets(ctemp, 2, infile)){
					incheck = 1;
					break;
				}
				block[(j * 4) + k] = *ctemp;
				sizecount++;
			}
		}
		
		//bitmap check
		pcheck = 0; 
		for(k = bitcheck[0]; k < SFS_BITBLOCKS(nbitblock); k++){
			if(pcheck == 1){
				break;
			}
			for(l = bitcheck[1]; l < 128; l++){
				if(pcheck == 1){
					break;
				}
				int check = bit[k][l];
				for(j = bitcheck[2]; j < 32; j++){
					if(!(BIT_CHECK(check, j))){
						BIT_SET(check, j);
						bit[k][l] = check;
						bitcheck[0] = k;
						bitcheck[1] = l;
						bitcheck[2] = j + 1;
						if(bitcheck[2] >= 32){
							bitcheck[2] = 0;
							bitcheck[1]++;
							if(bitcheck[1] >= 128){
								bitcheck[1] = 0;
								bitcheck[0]++;
							}
						}
						emptyinode = (k * 4096) + (l * 32) + j;
						pcheck++;
						if(pcheck == 1){
							break;
						}
					}
				}
			}
		}
        
		if(pcheck <= 0){
			error_message("cpin", local_path, -4);
			incheck = 1;
			break;
		}
		else{
			disk_write(block, emptyinode);	
			disk_read(&newthing, emptyi[0]);
			newthing.sfi_size += sizecount;
			newthing.sfi_direct[i] = emptyinode;
			disk_write(&newthing, emptyi[0]);
		}
	}

	//indirect
	if(incheck == 0){	
		pcheck = 0; 
		for(k = bitcheck[0]; k < SFS_BITBLOCKS(nbitblock); k++){
			if(pcheck == 1){
				break;
			}
			for(l = bitcheck[1]; l < 128; l++){
				if(pcheck == 1){
					break;
				}
				int check = bit[k][l];
				for(j = bitcheck[2]; j < 32; j++){
					if(!(BIT_CHECK(check, j))){
						BIT_SET(check, j);
						bit[k][l] = check;
						bitcheck[0] = k;
						bitcheck[1] = l;
						bitcheck[2] = j + 1;
						if(bitcheck[2] >= 32){
							bitcheck[2] = 0;
							bitcheck[1]++;
							if(bitcheck[1] >= 128){
								bitcheck[1] = 0;
								bitcheck[0]++;
							}
						}
						emptyinode = (k * 4096) + (l * 32) + j;
						pcheck++;
						if(pcheck == 1){
							break;
						}
					}
				}
			}
		}
		if(pcheck <= 0){
			error_message("cpin", local_path, -4);
			incheck = 1;
		}
		else{
			bzero(indblock, (sizeof(int)*128));
			disk_write(indblock, emptyinode);	
			disk_read(&newthing, emptyi[0]);
			newthing.sfi_indirect = emptyinode;
			disk_write(&newthing, emptyi[0]);
		}
		
		for(y = 0; y < 128; y++){
			if(incheck == 1){
				break;
			}
			bzero(block, (sizeof(char)*512));
			sizecount = 0;
			for(j = 0; j < 128; j++){
				if(incheck == 1){
					break;
				}
				for(k = 0; k < 4; k++){
					if(!fgets(ctemp, 2, infile)){
						incheck = 1;
						break;
					}
					block[(j * 4) + k] = *ctemp;
					sizecount++;
				}
			}
			
			//bitmap check
			pcheck = 0; 
			for(k = bitcheck[0]; k < SFS_BITBLOCKS(nbitblock); k++){
				if(pcheck == 1){
					break;
				}
				for(l = bitcheck[1]; l < 128; l++){
					if(pcheck == 1){
						break;
					}
					int check = bit[k][l];
					for(j = bitcheck[2]; j < 32; j++){
						if(!(BIT_CHECK(check, j))){
							BIT_SET(check, j);
							bit[k][l] = check;
							bitcheck[0] = k;
							bitcheck[1] = l;
							bitcheck[2] = j + 1;
							if(bitcheck[2] >= 32){
								bitcheck[2] = 0;
								bitcheck[1]++;
								if(bitcheck[1] >= 128){
									bitcheck[1] = 0;
									bitcheck[0]++;
								}
							}
							emptyinode = (k * 4096) + (l * 32) + j;
							pcheck++;
							if(pcheck == 1){
								break;
							}
						}
					}
				}
			}
                
			if(pcheck <= 0){
				error_message("cpin", local_path, -4);
				incheck = 1;
				break;
			}
			else{
				disk_write(block, emptyinode);	
				disk_read(&newthing, emptyi[0]);
				newthing.sfi_size += sizecount;
				disk_write(&newthing, emptyi[0]);
				disk_read(indblock, newthing.sfi_indirect);
				indblock[y] = emptyinode;
				disk_write(indblock, newthing.sfi_indirect);
			}
		}
	}
	
	for(i = 0; i < SFS_BITBLOCKS(nbitblock); i++){
		disk_write(bit[i], (SFS_MAP_LOCATION + i));
	}

	fclose(infile);
}

void sfs_cpout(const char* local_path, const char* path) 
{	
	int i, j;
	int pcheck = 0;
	struct sfs_inode si, check;
	disk_read(&si, sd_cwd.sfd_ino);

	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	for(i = 0; i < SFS_NDIRECT; i++){
		if(pcheck == 1){
			break;
		}
		if(si.sfi_direct[i] <= 0){
			continue;
		}
		disk_read(sd, si.sfi_direct[i]);
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino > 0){
				disk_read(&check, sd[j].sfd_ino);
				if(!strcmp(local_path, sd[j].sfd_name)){
					if(check.sfi_type == SFS_TYPE_DIR){
						error_message("cpout", local_path, -10);
						return;
					}
					disk_read(&si, sd[j].sfd_ino);
					pcheck = 1;
					break;
				}
			}
		}
	}
	if(pcheck == 0){
		error_message("cpout", local_path, -1);
		return;
	}
	
	//path already been check
	FILE *checkfile = fopen(path, "r");
	if(checkfile != NULL){
		error_message("cpout", path, -6);
		fclose(checkfile);
		return;
	}

	//read localpath and write path
	char bit[512];
	int indbit[128];
	int k;
	char temp[1];
	int incheck = 0;
	FILE *putfile = fopen(path, "wb");
	int sizecount = si.sfi_size;

	for(i = 0; i < SFS_NDIRECT; i++){
		if(si.sfi_direct[i] > 0){
			disk_read(bit, si.sfi_direct[i]);
			for(j = 0; j < 128; j++){
				for(k = 0; k < 4; k++){
					*temp = bit[(j * 4) + k];
					if(sizecount > 0){
						fwrite(temp, sizeof(char), 1, putfile);
						sizecount--;
					}
					else{
						incheck = 1;
						break;
					}	
				}
			}
		}
		else{
			incheck = 1;
			break;
		}
	}

	//indirect check
	int echeck = 0;
	if(incheck == 0){
		if(si.sfi_indirect > 0){
			disk_read(indbit, si.sfi_indirect);
			for(i = 0; i < 128; i++){
				if(echeck == 1){
					break;
				}
				if(indbit[i] > 0){
					disk_read(bit, indbit[i]);
					for(j = 0; j < 128; j++){
						if(echeck == 1){
							break;
						}
						for(k = 0; k < 4; k++){
							*temp = bit[(j * 4) + k];
							if(sizecount > 0){
								fwrite(temp, sizeof(char), 1, putfile);
								sizecount--;
							}
							else{
								echeck = 1;
								break;
							}
						}
					}
				}
				else{
					break;
				}
			}
		}
	}
	fclose(putfile);
}

void dump_inode(struct sfs_inode inode) {
	int i;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];

	printf("size %d type %d direct ", inode.sfi_size, inode.sfi_type);
	for(i=0; i < SFS_NDIRECT; i++) {
		printf(" %d ", inode.sfi_direct[i]);
	}
	printf(" indirect %d",inode.sfi_indirect);
	printf("\n");

	if (inode.sfi_type == SFS_TYPE_DIR) {
		for(i=0; i < SFS_NDIRECT; i++) {
			if (inode.sfi_direct[i] == 0) break;
			disk_read(dir_entry, inode.sfi_direct[i]);
			dump_directory(dir_entry);
		}
	}

}

void dump_directory(struct sfs_dir dir_entry[]) {
	int i;
	struct sfs_inode inode;
	for(i=0; i < SFS_DENTRYPERBLOCK;i++) {
		printf("%d %s\n",dir_entry[i].sfd_ino, dir_entry[i].sfd_name);
		disk_read(&inode,dir_entry[i].sfd_ino);
		if (inode.sfi_type == SFS_TYPE_FILE) {
			printf("\t");
			dump_inode(inode);
		}
	}
}

void sfs_dump() {
	// dump the current directory structure
	struct sfs_inode c_inode;

	disk_read(&c_inode, sd_cwd.sfd_ino);
	printf("cwd inode %d name %s\n",sd_cwd.sfd_ino,sd_cwd.sfd_name);
	dump_inode(c_inode);
	printf("\n");

}
