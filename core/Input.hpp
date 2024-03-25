#pragma once
#include <string>

using namespace std;

class Input
{
public:
	static Input& Get() 
	{
		static Input instance;
		return instance; 
	}

	bool GetKeyDown(string key_name){
		return false;
	}
	void SetKeyDown(string key_name) {}
private:
	
};