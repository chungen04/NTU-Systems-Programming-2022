#include "status.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "utility.h"
#include <stdbool.h>
#include <sys/wait.h>

int battle(Status* pssm1, Status* pssm2, char current_battle_id){
	pssm1->current_battle_id = current_battle_id;
	pssm2->current_battle_id = current_battle_id;
	if((pssm1->HP<pssm2->HP) || (pssm1->HP==pssm2->HP && pssm1->real_player_id<pssm2->real_player_id)){
		// 1 attacks 2
		pssm2->HP -= pssm1->ATK;
		if(pssm1->attr == battleattr(current_battle_id)){
			pssm2->HP -= pssm1->ATK;
		}
		// 2 attacks 1
		if(pssm2->HP > 0){
			pssm1->HP -= pssm2->ATK;
			if(pssm2->attr == battleattr(current_battle_id)){
				pssm1->HP -= pssm2->ATK;
			}
		}
	}else{
		//2 attacks 1
		pssm1->HP -= pssm2->ATK;
		if(pssm2->attr == battleattr(current_battle_id)){
			pssm1->HP -= pssm2->ATK;
		}
		//1 attacks 2
		if(pssm1->HP > 0){
			pssm2->HP -= pssm1->ATK;
			if(pssm1->attr == battleattr(current_battle_id)){
				pssm2->HP -= pssm1->ATK;
			}
		}
	}
	if(pssm1->HP*pssm2->HP<=0){
		pssm1->battle_ended_flag = 1;
		pssm2->battle_ended_flag = 1;
		if(pssm1->HP <= 0){
			return 0; // 0: pssm2 win
		}else{
			return 1; // 1: pssm1 win
		}
	}else{
		return -1;
	}
}

void write_log_from(int logfd, char battle_id, int pid, char dest_id, int dest_pid, Status* pssm){
	char buf[100];
	if(dest_id < 15){
		snprintf(buf, 100, "%c,%d pipe from %d,%d %d,%d,%c,%d\n", battle_id, pid, dest_id, dest_pid, pssm->real_player_id, pssm->HP, pssm->current_battle_id, pssm->battle_ended_flag);
	}else{
		snprintf(buf, 100, "%c,%d pipe from %c,%d %d,%d,%c,%d\n", battle_id, pid, dest_id, dest_pid, pssm->real_player_id, pssm->HP, pssm->current_battle_id, pssm->battle_ended_flag);
	}
	write(logfd, buf, strlen(buf));
}

void write_log_to(int logfd, char battle_id, int pid, char dest_id, int dest_pid, Status* pssm){
	char buf[100];
	if(dest_id < 15){
		snprintf(buf, 100, "%c,%d pipe to %d,%d %d,%d,%c,%d\n", battle_id, pid, dest_id, dest_pid, pssm->real_player_id, pssm->HP, pssm->current_battle_id, pssm->battle_ended_flag);
	}else{
		snprintf(buf, 100, "%c,%d pipe to %c,%d %d,%d,%c,%d\n", battle_id, pid, dest_id, dest_pid, pssm->real_player_id, pssm->HP, pssm->current_battle_id, pssm->battle_ended_flag);
	}
	write(logfd, buf, strlen(buf));
}

int main (int argc, char* argv[]) {
	//TODO
	char battle_id = argv[1][0];
	int parent_pid = atoi(argv[2]);
	
	// init mode

	char logpath[20];
	sprintf(logpath, "log_battle%c.txt", battle_id);
	
	int logfd = open(logpath, O_APPEND | O_WRONLY | O_CREAT, 0600);

	int fd1[2];
	int fd2[2];
	pipe(fd1);
	pipe(fd2);
	pid_t pid1;
	pid_t pid2;
	char battle_l = left_id(battle_id);
	char battle_r = right_id(battle_id);
	if((pid1 = fork()) == 0){
		// child 1: fd1[1] for write to parent, fd2[0] to read from parent
		close(fd1[0]);
		close(fd2[1]);
		dup2(fd1[1], STDOUT_FILENO);
		dup2(fd2[0], STDIN_FILENO);
		char buf[100];
		snprintf(buf, 100, "%d", getppid());
		char id[5];
		id[0] = left_id(battle_id);
		char* arg_vector[4];
		if('A' <= id[0] && id[0] <= 'N'){
			arg_vector[0] = "./battle";
			id[1] = '\0';
			arg_vector[1] = id;
			arg_vector[2] = buf;
			arg_vector[3] = NULL;
			execve("./battle", arg_vector, NULL);
		}else{ // exec players, not battles
			snprintf(id, 5, "%d", left_id(battle_id));
			arg_vector[0] = "./player";
			arg_vector[1] = id;
			arg_vector[2] = buf;
			arg_vector[3] = NULL;
			execve("./player", arg_vector, NULL);
		}
		exit(0); // need this?
	}
	// parent: fd1[0] to read from child 1, fd2[1] to write to child 1
	close(fd1[1]);
	close(fd2[0]);
	int fd3[2];
	int fd4[2];
	pipe(fd3);
	pipe(fd4);
	if((pid2 = fork()) == 0){
		// child 2: fd3[1] for write to parent, fd4[0] to read from parent
		close(fd3[0]);
		close(fd4[1]);
		dup2(fd3[1], STDOUT_FILENO);
		dup2(fd4[0], STDIN_FILENO);
		char buf[100];
		snprintf(buf, 100, "%d", getppid());
		char id[5];
		id[0] = right_id(battle_id);
		char* arg_vector[4];
		if('A' <= id[0] && id[0] <= 'N'){
			arg_vector[0] = "./battle";
			id[1] = '\0';
			arg_vector[1] = id;
			arg_vector[2] = buf;
			arg_vector[3] = NULL;
			execve("./battle", arg_vector, NULL);
		}else{ // exec players, not battles
			snprintf(id, 5, "%d", right_id(battle_id));
			arg_vector[0] = "./player";
			arg_vector[1] = id;
			arg_vector[2] = buf;
			arg_vector[3] = NULL;
			execve("./player", arg_vector, NULL);
		}
		exit(0);
	}

	// parent: fd3[0] to read from child 2, fd4[1] to write to child 2
	close(fd3[1]);
	close(fd4[0]);

	// waiting mode: 
	Status pssm_1;
	Status pssm_2;

	// playing mode
	// perform battle
	int winner = -1; // 0 for left child win, 1 for right child win
	while(winner == -1){
		read(fd1[0], &pssm_1, sizeof(Status));
		write_log_from(logfd, battle_id, getpid(), left_id(battle_id), pid1, &pssm_1);
		
		read(fd3[0], &pssm_2, sizeof(Status));
		write_log_from(logfd, battle_id, getpid(), right_id(battle_id), pid2, &pssm_2);
		
		winner = battle(&pssm_1, &pssm_2, battle_id);
		// send pssms back. 
		
		write(fd2[1], &pssm_1, sizeof(Status));
		write_log_to(logfd, battle_id, getpid(), left_id(battle_id), pid1, &pssm_1);
		
		write(fd4[1], &pssm_2, sizeof(Status));
		write_log_to(logfd, battle_id, getpid(), right_id(battle_id), pid2, &pssm_2);
	}
	wait(NULL);

	//passing mode
	Status pssm_3;
	pssm_3.HP = 1;
	if(battle_id != 'A'){
		int winner_fd_read = winner? fd1[0]:fd3[0];
		int winner_fd_write = winner? fd2[1]:fd4[1];
		char winner_child = winner? left_id(battle_id):right_id(battle_id);
		pid_t winner_child_pid = winner? pid1:pid2;
		while(pssm_3.HP > 0){
			read(winner_fd_read, &pssm_3, sizeof(Status));
			write_log_from(logfd, battle_id, getpid(), winner_child , winner_child_pid, &pssm_3);
			
			write(STDOUT_FILENO, &pssm_3, sizeof(Status));
			write_log_to(logfd, battle_id, getpid(), get_parent_id(battle_id), getppid(), &pssm_3);
			
			read(STDIN_FILENO, &pssm_3, sizeof(Status));
			write_log_from(logfd, battle_id, getpid(), get_parent_id(battle_id), getppid(), &pssm_3);
			
			write(winner_fd_write, &pssm_3, sizeof(Status));
			write_log_to(logfd, battle_id, getpid(), winner_child, winner_child_pid, &pssm_3);

			//peek at pssm. if pssm's battle id is A and ended flag is true, also break the loop
			if(pssm_3.battle_ended_flag && pssm_3.current_battle_id == 'A'){
				break;
			}
		}
	}else{
		// A has no passing mode.
		char buf[100];
		int winner = winner? pssm_1.real_player_id: pssm_2.real_player_id;
		snprintf(buf, 100, "Champion is P%d\n", winner);
		write(STDOUT_FILENO, buf, strlen(buf));
	}
	wait(NULL);

	close(fd1[0]);
	close(fd3[0]);
	close(fd2[1]);
	close(fd4[1]);
	close(logfd);
	exit(0);
}