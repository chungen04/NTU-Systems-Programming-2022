#include "status.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "utility.h"
#include <stdbool.h>
#include <errno.h>

void write_log_1(int logfd, int id, Status* pssm){
	//Right after the player receives PSSM from the target (parent).
	// [self's ID],[self's pid] pipe from [target's ID],[parent's pid] [real_player_id],[HP],[current_battle_id],[battle_ended_flag]\n
	char buf[100];
	snprintf(buf, 100, "%d,%d pipe from %c,%d %d,%d,%c,%d\n", id, getpid(), player_parent(id), getppid(), pssm->real_player_id, pssm->HP, pssm->current_battle_id, pssm->battle_ended_flag);
	write(logfd, buf, strlen(buf));
}

void write_log_2(int logfd, int id, Status* pssm){
	// Right before the player sends PSSM to the target (parent)
	//[self's ID],[self's pid] pipe to [target's ID],[target's pid] [real_player_id],[HP],[current_battle_id],[battle_ended_flag]\n
	char buf[100];
	snprintf(buf, 100, "%d,%d pipe to %c,%d %d,%d,%c,%d\n", id, getpid(), player_parent(id), getppid(), pssm->real_player_id, pssm->HP, pssm->current_battle_id, pssm->battle_ended_flag);
	write(logfd, buf, strlen(buf));
}

void write_log_3(int logfd, int id, Status* pssm){
	//Right before the Real Player sends PSSM to the target (agent player). (Notice that this log format is slightly different to others)
	//[self's ID],[self's pid] fifo to [target's id] [real_player_id],[HP]\n
	char buf[100];
	snprintf(buf, 100, "%d,%d fifo to %d %d,%d\n", id, getpid(), agent(pssm->current_battle_id), pssm->real_player_id, pssm->HP);
	write(logfd, buf, strlen(buf));
}

void write_log_4(int logfd, int id, Status* pssm){
	// Right after the Agent Player receives PSSM from the target (real player). (Notice that this log format is slightly different to others)
	//[self's ID],[self's pid] fifo from [target's id] [real_player_id],[HP]\n
	char buf[100];
	snprintf(buf, 100, "%d,%d fifo from %d %d,%d\n", id, getpid(), pssm->real_player_id, pssm->real_player_id, pssm->HP);
	write(logfd, buf, strlen(buf));
}

int main (int argc, char* argv[]) {
	//TODO
	int player_id = atoi(argv[1]);
	int parent_pid = atoi(argv[2]);
	// stdin and stdout are already for read/writing to parent
	Status pssm;
	
	if(player_id <= 7){
		init(player_id, &pssm);
	}else{
		char buf[100];
		snprintf(buf, 100, "player%d.fifo", player_id);
		mkfifo(buf, 0600);
		
		int fifofd = open(buf, O_RDONLY);
		while(read(fifofd, &pssm, sizeof(Status))<1){
			continue;
		}; // blocked, read until something is read
	}

	char logpath[16];
	sprintf(logpath, "log_player%d.txt", pssm.real_player_id);
	int logfd = open(logpath, O_APPEND | O_WRONLY | O_CREAT, 0600);
	if(player_id >= 8){
		write_log_4(logfd, player_id, &pssm);
		pssm.battle_ended_flag = 0;
	}
	
	while(1){
		write(STDOUT_FILENO, &pssm, sizeof(Status));
		write_log_2(logfd, player_id, &pssm);
		read(STDIN_FILENO, &pssm, sizeof(Status));
		write_log_1(logfd, player_id, &pssm);
		if(pssm.battle_ended_flag){
			if(pssm.current_battle_id == 'A'){
				close(logfd);
				exit(0);
			} // the whole tournament ended 
			if(pssm.HP > 0){
				pssm.HP = (get_max_HP(pssm.real_player_id)+pssm.HP)/2;
				pssm.battle_ended_flag = 0;
			}else{
				pssm.battle_ended_flag = 0;
				break;
			}
		}
	}
	//lose
	//if it's in zone A, recover with full blood
	// write pssm to fifo
	//exit
	if(zone(pssm.current_battle_id) == 'B'){
		close(logfd);
		exit(0);
	}else{
		pssm.HP = get_max_HP(pssm.real_player_id);
		char buf[100];
		snprintf(buf, 100, "player%d.fifo", agent(pssm.current_battle_id));
		int fd;
		while((fd = open(buf, O_WRONLY)) == -1){
			continue;
		} // check the fifo is opened
		write_log_3(logfd, player_id, &pssm);
		write(fd, &pssm, sizeof(Status));
		close(logfd);
		exit(0);
	}
	close(logfd);
	exit(0);
}