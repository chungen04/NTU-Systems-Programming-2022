#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "status.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

char left_id(char id){
	// return the number if the battle is leaf.
	// return 127 for default case
	switch(id){
		case 'A':
			return 'B';
			break;
		case 'B':
			return 'D';
			break;
		case 'D':
			return 'G';
			break;
		case 'G':
			return 0;
			break;
		case 'H':
			return 2;
			break;
		case 'E':
			return 'I';
			break;
		case 'I':
			return 4;
			break;
		case 'J':
			return 6;
			break;
		case 'C':
			return 'F';
			break;
		case 'F':
			return 'K';
			break;
		case 'K':
			return 'M';
			break;
		case 'M':
			return 8;
			break;
		case 'N':
			return 11;
			break;
		case 'L':
			return 'N';
			break;
		default:
			return 127;
			break; 
	}
}

char right_id(char id){
	// return the number if the battle is leaf.
	// return 127 for default case
	switch(id){
		case 'A':
			return 'C';
			break;
		case 'B':
			return 'E';
			break;
		case 'D':
			return 'H';
			break;
		case 'G':
			return 1;
			break;
		case 'H':
			return 3;
			break;
		case 'E':
			return 'J';
			break;
		case 'I':
			return 5;
			break;
		case 'J':
			return 7;
			break;
		case 'C':
			return 14;
			break;
		case 'F':
			return 'L';
			break;
		case 'K':
			return 10;
			break;
		case 'M':
			return 9;
			break;
		case 'N':
			return 12;
			break;
		case 'L':
			return 13;
			break;
		default:
			return 127;
			break; 
	}
}

Attribute battleattr(char battleid){
	switch(battleid){
		case 'A':
			return FIRE;
			break;
		case 'B':
			return GRASS;
			break;
		case 'C':
			return WATER;
			break;
		case 'D':
			return WATER;
			break;
		case 'E':
			return FIRE;
			break;
		case 'F':
			return FIRE;
			break;
		case 'G':
			return FIRE;
			break;
		case 'H':
			return GRASS;
			break;
		case 'I':
			return WATER;
			break;
		case 'J':
			return GRASS;
			break;
		case 'K':
			return GRASS;
			break;
		case 'L':
			return GRASS;
			break;
		case 'M':
			return FIRE;
			break;
		case 'N':
			return WATER;
			break;
		default:
			break;
	}
	return 0;	
}

char get_parent_id(char id){
	switch(id){
		case 'A':
			return 0;
			break;
		case 'B':
			return 'A';
			break;
		case 'C':
			return 'A';
			break;
		case 'D':
			return 'B';
			break;
		case 'E':
			return 'B';
			break;
		case 'F':
			return 'C';
			break;
		case 'G':
			return 'D';
			break;
		case 'H':
			return 'D';
			break;
		case 'I':
			return 'E';
			break;
		case 'J':
			return 'E';
			break;
		case 'K':
			return 'F';
			break;
		case 'L':
			return 'F';
			break;
		case 'M':
			return 'K';
			break;
		case 'N':
			return 'L';
			break;
		default:
			break;
	}
	return 0;
}

void init(int player_id, Status* pssm){
	// write player information into pssm.
	FILE* infile = fopen("player_status.txt", "r");
	fseek(infile, 0, SEEK_SET);
	char tmp[10];
	for(int i=0; i<=player_id; i++){
		fscanf(infile, "%d %d %s %c %d", &(pssm->HP), &(pssm->ATK), tmp, &(pssm->current_battle_id), &(pssm->battle_ended_flag));
		if(!strcmp(tmp, "GRASS")){
			pssm->attr = GRASS;
		}
		if(!strcmp(tmp, "WATER")){
			pssm->attr = WATER;
		}
		if(!strcmp(tmp, "FIRE")){
			pssm->attr = FIRE;
		}
	}
	pssm->real_player_id = player_id;
	fclose(infile);
}

int get_max_HP(int player_id){
	FILE* infile = fopen("player_status.txt", "r");
	fseek(infile, 0, SEEK_SET);
	char tmp[10];
	int dump1, dump3;
	char dump2;
	int hp;
	for(int i=0; i<=player_id; i++){
		fscanf(infile, "%d %d %s %c %d", &hp, &dump1, tmp, &dump2, &dump3);
	}
	fclose(infile);
	return hp;
}

char zone(char battle_id){
	switch(battle_id){
		case 'A':
			return 'B';
			break;
		case 'B':
			return 'A';
			break;
		case 'C':
			return 'B';
			break;
		case 'D':
			return 'A';
			break;
		case 'E':
			return 'A';
			break;
		case 'F':
			return 'B';
			break;
		case 'G':
			return 'A';
			break;
		case 'H':
			return 'A';
			break;
		case 'I':
			return 'A';
			break;
		case 'J':
			return 'A';
			break;
		case 'K':
			return 'B';
			break;
		case 'L':
			return 'B';
			break;
		case 'M':
			return 'B';
			break;
		case 'N':
			return 'B';
			break;
		default:
			break;
	}
	return 0;
}

int agent(char battle_id){
	switch (battle_id){
		case 'B':
			return 14;
			break;
		case 'D':
			return 10;
			break;
		case 'E':
			return 13;
			break;
		case 'G':
			return 8;
			break;
		case 'H':
			return 11;
			break;
		case 'I':
			return 9;
			break;
		case 'J':
			return 12;
			break;
		default:
			break;
	}
	return 0;
}

char player_parent(int player){
	switch (player)	{
	case 0:
		return 'G';
		break;
	case 1:
		return 'G';
		break;
	case 2:
		return 'H';
		break;
	case 3:
		return 'H';
		break;
	case 4:
		return 'I';
		break;
	case 5:
		return 'I';
		break;
	case 6:
		return 'J';
		break;
	case 7:
		return 'J';
		break;
	case 8:
		return 'M';
		break;
	case 9:
		return 'M';
		break;
	case 10:
		return 'K';
		break;
	case 11:
		return 'N';
		break;
	case 12:
		return 'N';
		break;
	case 13:
		return 'L';
		break;
	case 14:
		return 'C';
		break;
	default:
		break;
	}
	return 0;
}

#endif