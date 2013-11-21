/*
Copyright (C) 1997-2003 Bleeding Eye Studio, Bryan Miller.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/



#include "gl_newshellfx.h"




int SkipLine(int *Index, char* DataBuffer, int Length){
	while (DataBuffer[*Index] != '\r' && DataBuffer[*Index] != '\n'){
		*Index += 1;
		if (*Index >= Length) return -1;
	}
	return 1;
}


int SkipWhiteSpace(int *Index, char* DataBuffer, int Length){
	while (DataBuffer[*Index] == ' ' || DataBuffer[*Index] == '\r' || 
		DataBuffer[*Index] == '\n' || DataBuffer[*Index] == '#'){
		
		if (DataBuffer[*Index] == '#'){
			if (SkipLine(Index, DataBuffer, Length) == -1)
				return -1;
		}
		else{
			*Index += 1;
			if (*Index >= Length)
				return -1;
		}
	}
	return 1;
}

int AlignToToken(int *Index, char* DataBuffer, int Length){
	while ((DataBuffer[*Index] < 'a' || DataBuffer[*Index] > 'z') && 
		(DataBuffer[*Index] < 'A' || DataBuffer[*Index] > 'Z') && 
		DataBuffer[*Index] != '{' && DataBuffer[*Index] != '}' && DataBuffer[*Index] != ';'){
		if (SkipWhiteSpace(Index, DataBuffer, Length) == -1)
			return -1;
	}
	return 1;
}

//Return values and their meanings
//-1 = Error in processing
//0 = Unknown Tag (not neccessarily an error)
//1 = TypeInfo tag
//2 = Stage Tag
int ParseBlockName(int *Index, char* DataBuffer, int Length, int *Value){
	char Token[1024];
	char value[5];
	int TokenIndex = 0;
	//int ValueIndex = 0;

	if (AlignToToken(Index, DataBuffer, Length) == -1)
		return -1;
	if (DataBuffer[*Index] == ';'){
		*Index += 1;
		return 0;
	}

	memset(Token, 0, 1024);
	memset(value, 0, 5);

	while (DataBuffer[*Index] != ' ' && DataBuffer[*Index] != '{'){
		Token[TokenIndex] = DataBuffer[*Index];
		*Index += 1;
		TokenIndex++;
		if (*Index >= Length) return -1;
		if (TokenIndex >= 1024) return -1;
	}

	if (strcmp(Token, "TypeInfo") == 0){
		*Value = 0;
		return 1;
	}
	else if (strstr(Token, "StageDef_ID_") != NULL){
		value[0] = Token[strlen(Token)-1];
		*Value = atoi(value);
		return 2;
	}
	else{
		*Value = 0;
		return 0;
	}

	return -1;
}



//Return values and their meanings
//-1 = Error in processing
//0 = Unknown Element (not neccessarily an error)
//1 = Start Block Element
//2 = End Block Element
//3 = NumStages
//4 = StartStage
//5 = StartSize
//6 = EndSize
//7 = SizeSpeed
//8 = StartAlpha
//9 = EndAlpha
//10 = AlphaSpeed
//11 = StageWait
int ParseElement(int *Index, char* DataBuffer, int Length, char* Value){
	char Token[1024];
	int TokenIndex = 0;
	int ValueIndex = 0;
	//int ReturnCode = 0;

	if (AlignToToken(Index, DataBuffer, Length) == -1)
		return -1;
	if (DataBuffer[*Index] == '{'){
		*Index += 1;
		return 1;
	}
	if (DataBuffer[*Index] == '}'){
		*Index += 1;
		return 2;
	}
	if (DataBuffer[*Index] == ';'){
		*Index += 1;
		return 0;
	}

	memset(Token, 0, 1024);

	while (DataBuffer[*Index] != ' ' && DataBuffer[*Index] != '{'){
		Token[TokenIndex] = DataBuffer[*Index];
		*Index += 1;
		TokenIndex++;
		if (*Index >= Length) return -1;
		if (TokenIndex >= 1024) return -1;
	}

	if (SkipWhiteSpace(Index, DataBuffer, Length) == -1)
		return -1;
	if (DataBuffer[*Index] != '=')
		return -1;
	else *Index += 1;
	if (SkipWhiteSpace(Index, DataBuffer, Length) == -1)
		return -1;

	while (DataBuffer[*Index] != ';' && DataBuffer[*Index] != '\r' && DataBuffer[*Index] != '\n'){
		Value[ValueIndex] = DataBuffer[*Index];
		*Index += 1;
		ValueIndex++;
		if (*Index >= Length) return -1;
		if (ValueIndex >= 100) return -1;
	}
	if (DataBuffer[*Index] != ';')
		return -1;
	else
		*Index += 1;


	if (strcmp(Token, "NumStages") == 0)
		return 3;
	if (strcmp(Token, "StartStage") == 0)
		return 4;
	if (strcmp(Token, "StartSize") == 0)
		return 5;
	if (strcmp(Token, "EndSize") == 0)
		return 6;
	if (strcmp(Token, "SizeSpeed") == 0)
		return 7;
	if (strcmp(Token, "StartAlpha") == 0)
		return 8;
	if (strcmp(Token, "EndAlpha") == 0)
		return 9;
	if (strcmp(Token, "AlphaSpeed") == 0)
		return 10;
	if (strcmp(Token, "StageWait") == 0)
		return 11;

	return 0;
}



//Return values and their meanings:
//-2 = ShellDef not found or Invalid definition file.
//-1 = Type out of bounds
//0 = Type already defined
//1 = Type loaded
int loadShellType(int Type){
	int FileLength;
	int DataIndex;
	char Filename[1024];
	char Header[1024];
	char *DataBuffer;
	char StrVal[100];
	int Value;
	int HeaderIndex = 0;
	int Result = 0;
	int i = 0;

	//Checking to see if the ShellTypes have been initilized to "not loaded". If not, we
	//do that first.
	if (ShellTypesSet != 1){
		int i = 0;
		for ( i = 0; i < MAX_SHELL_TYPES; i++){
			ShellType[i].Loaded = 0;
		}
		ShellTypesSet = 1;
	}

	//Then we make sure the Type requested is valid and has not been loaded yet.
	if (Type < 0 || Type >= MAX_SHELL_TYPES) return -1;
	if (ShellType[Type].Loaded == 1) return 0; //If the type has been loaded, we return

	//We then check to make sure the file is there.
	if (Type == 0)
		strcpy(Filename, "/ShellDef/god_shell.sdf");
	else if (Type == 1)
		strcpy(Filename, "/ShellDef/inq_shell.sdf");
	else if (Type == 2)
		strcpy(Filename, "/ShellDef/inv_shell.sdf");
	else if (Type == 3)
		strcpy(Filename, "/ShellDef/quad_shell.sdf");
	else if (Type == 4)
		strcpy(Filename, "/ShellDef/gnrc_shell.sdf");
	FileLength = ri.FS_LoadFile(Filename, NULL);
	if (FileLength < 1) return -2; //If it isn't, we return.

	//If the file does exist, we load in the file
	ri.FS_LoadFile(Filename, &DataBuffer);
	DataIndex = 0;

	memset(Header, 0, 1024);
	while (DataBuffer[DataIndex] != ' '){
		Header[HeaderIndex] = DataBuffer[DataIndex];
		DataIndex++;
		HeaderIndex++;
		if (DataIndex >= FileLength)
			return -2;
		if (HeaderIndex >= 1024)
			return -2;
	}
	if (strcmp(Header, "q2_shell_def") != 0)
		return -2;
	DataIndex++;
	memset(Header, 0, 1024);
	HeaderIndex = 0;
	while (DataBuffer[DataIndex] != ' '){
		Header[HeaderIndex] = DataBuffer[DataIndex];
		DataIndex++;
		HeaderIndex++;
		if (DataIndex >= FileLength)
			return -2;
		if (HeaderIndex >= 1024)
			return -2;
	}
	if (strcmp(Header, "v1.0") != 0)
		return -2;
	

	Result = ParseBlockName(&DataIndex, DataBuffer, FileLength, &Value);
	if (Result != 1)
		return -2;
	//Checking to make sure there is an open bracket
	Result = ParseElement(&DataIndex, DataBuffer, FileLength, StrVal);
	if (Result != 1)
		return -2;

	memset(StrVal, 0, 100);
	Result = ParseElement(&DataIndex, DataBuffer, FileLength, StrVal);
	while (Result != 2){
		if (Result == -1) return -1;
		if (Result == 3){
			ShellType[Type].NumStages = atoi(StrVal);
			if (ShellType[Type].NumStages < 0 || ShellType[Type].NumStages >= MAX_SHELL_STAGES)
				return -2;
		}
		else if (Result == 4){
			ShellType[Type].StartStage = atoi(StrVal) - 1;
			if (ShellType[Type].StartStage < 0)
				return -2;
		}
		memset(StrVal, 0, 100);
		Result = ParseElement(&DataIndex, DataBuffer, FileLength, StrVal);
	}


	for (i = 0; i < ShellType[Type].NumStages; i++){
		Result = ParseBlockName(&DataIndex, DataBuffer, FileLength, &Value);
		if (Result < 0) return -1;
		if (Result == 2){
			int WaitMarked = 0;
			int FoundSSize = 0;
			int FoundESize = 0;
			int FoundSSpeed = 0;
			int FoundSAlpha = 0;
			int FoundEAlpha = 0;
			int FoundASpeed = 0;
			memset(StrVal, 0, 100);
			Result = ParseElement(&DataIndex, DataBuffer, FileLength, StrVal);
			if (Result != 1)
				return -1;
			memset(StrVal, 0, 100);
			Result = ParseElement(&DataIndex, DataBuffer, FileLength, StrVal);
			while (Result != 2){
				if (Result == 5){//StartSize
					ShellType[Type].Stage[i].StartSize = (float)atof(StrVal);
					FoundSSize = 1;
				}
				else if (Result == 6){//EndSize
					ShellType[Type].Stage[i].EndSize = (float)atof(StrVal);
					FoundESize = 1;
				}
				else if (Result == 7){//SizeSpeed
					ShellType[Type].Stage[i].SizeDivisor = (float)atof(StrVal);
					FoundSSpeed = 1;
				}
				else if (Result == 8){//StartAlpha
					ShellType[Type].Stage[i].StartAlpha = (float)atof(StrVal);
					FoundSAlpha = 1;
				}
				else if (Result == 9){//EndAlpha
					ShellType[Type].Stage[i].EndAlpha = (float)atof(StrVal);
					FoundEAlpha = 1;
				}
				else if (Result == 10){//AlphaSpeed
					ShellType[Type].Stage[i].AlphaDivisor = (float)atof(StrVal);
					FoundASpeed = 1;
				}
				else if (Result == 11){//StageWait
					ShellType[Type].Stage[i].Wait = (float)atof(StrVal);
					WaitMarked = 1;
				}
				memset(StrVal, 0, 100);
				Result = ParseElement(&DataIndex, DataBuffer, FileLength, StrVal);
			}
			if (FoundSSize != 1)
				return -1;
			if (FoundSAlpha != 1)
				return -1;
			
			if (WaitMarked != 1){
				if (FoundESize == 0)
					return -1;
				if (FoundSSpeed == 0)
					return -1;
				if (FoundEAlpha == 0)
					return -1;
				if (FoundASpeed == 0)
					return -1;
			}
		}
	}

	return 1;
}


//return values and their meanings...
//0 = God Shell
//1 = Invulnerabiltiy and Quad combo shell
//1 = Invulnerability Shell
//2 = Quad Damage Shell
//3 = Unknown (Generic)
int determineShellType(const int flags){
	//( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM)
	if ((flags & RF_SHELL_RED) && (flags & RF_SHELL_GREEN) && flags & (RF_SHELL_BLUE))
		return 0;
	else if ((flags & RF_SHELL_RED) && (flags & RF_SHELL_BLUE))
		return 1;
	else if ((flags & RF_SHELL_RED))
		return 2;
	else if ((flags & RF_SHELL_BLUE))
		return 3;
	return 4;
}



void generateValues(int Type, int ShellIndex){
	int SIndex;
	float SDist;
	float SDiv;
	float ADist;
	float ADiv;
	if (Type < 0 || Type >= MAX_SHELL_TYPES) return;

	SIndex = ActiveShells[ShellIndex].Stage;
	if (ShellType[Type].Stage[SIndex].Wait != 0){
		if (ActiveShells[ShellIndex].fading < ShellType[Type].Stage[SIndex].Wait)
			ActiveShells[ShellIndex].fading += 1.0;
		else{
			ActiveShells[ShellIndex].fading = 0.0;
			ActiveShells[ShellIndex].Stage++;
			if (ActiveShells[ShellIndex].Stage >= ShellType[Type].NumStages)
				ActiveShells[ShellIndex].Stage = 0;
			SIndex = ActiveShells[ShellIndex].Stage;
			ActiveShells[ShellIndex].size = ShellType[Type].Stage[SIndex].StartSize;
			ActiveShells[ShellIndex].alpha = ShellType[Type].Stage[SIndex].StartAlpha;
		}
		return;
	}

	SDist = ShellType[Type].Stage[SIndex].EndSize - ShellType[Type].Stage[SIndex].StartSize;
	ADist = ShellType[Type].Stage[SIndex].EndAlpha - ShellType[Type].Stage[SIndex].StartAlpha;
	SDiv = 60.0 * ShellType[Type].Stage[SIndex].SizeDivisor;
	ADiv = 60.0 * ShellType[Type].Stage[SIndex].AlphaDivisor;

	ActiveShells[ShellIndex].size += (SDist/SDiv);
	ActiveShells[ShellIndex].alpha += (ADist/ADiv);

	if (SDist >= 0.0){
		if (ActiveShells[ShellIndex].size > ShellType[Type].Stage[SIndex].EndSize)
			ActiveShells[ShellIndex].size = ShellType[Type].Stage[SIndex].EndSize;
	}
	else{
		if (ActiveShells[ShellIndex].size < ShellType[Type].Stage[SIndex].EndSize)
			ActiveShells[ShellIndex].size = ShellType[Type].Stage[SIndex].EndSize;
	}

	if (ADist >= 0.0){
		if (ActiveShells[ShellIndex].alpha > ShellType[Type].Stage[SIndex].EndAlpha)
			ActiveShells[ShellIndex].alpha = ShellType[Type].Stage[SIndex].EndAlpha;
	}
	else{
		if (ActiveShells[ShellIndex].alpha < ShellType[Type].Stage[SIndex].EndAlpha)
			ActiveShells[ShellIndex].alpha = ShellType[Type].Stage[SIndex].EndAlpha;
	}

	if (ActiveShells[ShellIndex].alpha == ShellType[Type].Stage[SIndex].EndAlpha &&
		ActiveShells[ShellIndex].size == ShellType[Type].Stage[SIndex].EndSize){

		ActiveShells[ShellIndex].fading = 0.0;
		ActiveShells[ShellIndex].Stage++;
		if (ActiveShells[ShellIndex].Stage >= ShellType[Type].NumStages)
			ActiveShells[ShellIndex].Stage = 0;
		SIndex = ActiveShells[ShellIndex].Stage;
		ActiveShells[ShellIndex].size = ShellType[Type].Stage[SIndex].StartSize;
		ActiveShells[ShellIndex].alpha = ShellType[Type].Stage[SIndex].StartAlpha;
	}
}


void generateShellType(int Type, int ShellIndex){
	if (ShellIndex < 0 || ShellIndex >= NumActiveShells)
		return;
	if (Type < 0 || Type >= MAX_SHELL_TYPES)
		return;
	if (loadShellType(Type) < 0){
		ShellType[Type].NumStages = 1;
		ShellType[Type].StartStage = 0;
		ShellType[Type].Stage[0].StartSize = 1.05;
		ShellType[Type].Stage[0].EndSize = 10.0;
		ShellType[Type].Stage[0].SizeDivisor = 1.5;
		ShellType[Type].Stage[0].StartAlpha = 1.0;
		ShellType[Type].Stage[0].EndAlpha = 0.0;
		ShellType[Type].Stage[0].AlphaDivisor = 1.5;
		ShellType[Type].Stage[0].Wait = 0;
		ShellType[Type].Loaded = 1;
	}
	if (ActiveShells[ShellIndex].Type != Type){
		int SIndex = ShellType[Type].StartStage;
		ActiveShells[ShellIndex].Type = Type;
		ActiveShells[ShellIndex].Stage = ShellType[Type].StartStage;
		ActiveShells[ShellIndex].alpha = ShellType[Type].Stage[SIndex].StartAlpha;
		ActiveShells[ShellIndex].size = ShellType[Type].Stage[SIndex].StartSize;
		ActiveShells[ShellIndex].fading = 0.0;
	}
	generateValues(Type, ShellIndex);
}


int activateShell(char *name){
	int i;
	if (name == NULL) return -1;
	if (NumActiveShells >= MAX_ACTIVE_SHELLS) return -1;

	if (NumActiveShells < 0 || NumActiveShells >= MAX_ACTIVE_SHELLS)
		NumActiveShells = 0;

	if (NumActiveShells > 0){
		for (i = 0; i < NumActiveShells; i++){
			if (strcmp(name, ActiveShells[i].name) == 0)
				return i;
		}
	}

	strcpy(ActiveShells[NumActiveShells].name, name);
	ActiveShells[NumActiveShells].size = 1.0;
	ActiveShells[NumActiveShells].alpha = 1.0;
	ActiveShells[NumActiveShells].fading = 0.0;
	ActiveShells[NumActiveShells].Stage = 0;
	ActiveShells[NumActiveShells].Type = 0;
	NumActiveShells++;

	return NumActiveShells - 1;
}


int getShellID(char* name){
	int i = 0;
	if (name == NULL) return -1;

	for (i = 0; i < NumActiveShells; i++){
		if (strcmp(ActiveShells[i].name, name) == 0)
			return i;
	}
	return -1;
}

//This should be the ONLY function called from the outside world. This does all the
//work needed.
void generateShellValues(int ShellIndex, float *size, float *alpha, float *fading, const int flags){
	if (ShellIndex < 0 || ShellIndex >= MAX_ACTIVE_SHELLS) return;

	switch (determineShellType(flags)){
		case 0: //God Shell
			generateShellType(0, ShellIndex);
			break;

		case 1: //Invulnerability and Quad Damage Combo Shell
			generateShellType(1, ShellIndex);
			break;

		case 2: //Invulnerability Shell
			generateShellType(2, ShellIndex);
			break;

		case 3: //Quad Damage Shell
			generateShellType(3, ShellIndex);
			break;

		default: //Generic Shell
			generateShellType(4, ShellIndex);
			break;
	}

	*size = ActiveShells[ShellIndex].size;
	*alpha = ActiveShells[ShellIndex].alpha;
	*fading = ActiveShells[ShellIndex].fading;
}








