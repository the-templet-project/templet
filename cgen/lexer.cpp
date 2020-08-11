/*--------------------------------------------------------------------------*/
/*  Copyright 2016 Sergei Vostokin                                          */
/*                                                                          */
/*  Licensed under the Apache License, Version 2.0 (the "License");         */
/*  you may not use this file except in compliance with the License.        */
/*  You may obtain a copy of the License at                                 */
/*                                                                          */
/*  http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                          */
/*  Unless required by applicable law or agreed to in writing, software     */
/*  distributed under the License is distributed on an "AS IS" BASIS,       */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*/
/*  See the License for the specific language governing permissions and     */
/*  limitations under the License.                                          */
/*--------------------------------------------------------------------------*/

#include <string>

static std::string str;
static std::string lastlexem;
static bool is_unget;
static int str_index;
static int len;

enum  State {_S_,_A_,_B_,_C_,_D_,_P_,_F_,_E_};

State Sta;

void lexinit(std::string&s)
{
	str = s;
	str_index = 0;
	is_unget = false;
	lastlexem.clear();
	len = str.length();
}

bool getlex(std::string&lex)
{
	if (is_unget){
		lex = lastlexem;
		is_unget = false;
		return true;
	}
	
	std::string lexem = "";
	Sta = _S_;
	char symb;
	while ((Sta !=_F_) && (str_index < len))
	{
		symb = str[str_index];
		switch (Sta)
		{
		case _S_:
			if (symb == ' ')
			{
				Sta = _S_;
				str_index++;
			}
			else
			{
				if (symb == '\\')
				{
					Sta = _A_;
					str_index++;
				}
				else
				{
					if (symb == '_')
					{
						lexem += symb;
						Sta = _D_;
						str_index++;
					}
					else
					{
						if (((symb >= 'a') && (symb <= 'z')) || ((symb >= 'A') && (symb <= 'Z')))
						{
							lexem += symb;
							Sta = _C_;
							str_index++;
						}
						else
						{
							lexem = symb;
							str_index++;
							Sta = _F_;
						}
					}
				}
			}
			break;
		case _A_:
			if ((symb == '\r') || (symb == '\t') || (symb == '_'))
			{
				Sta = _A_;
				str_index++;
			}
			else
				if (symb == '\n')
				{
					Sta = _S_;
					str_index++;
				}
				else
				{
					lexem = '\\';
					Sta = _F_;
				}
			break;
		case _D_:
			if (symb == '_') 
			{
				lexem += symb;
				Sta = _D_;
				str_index++;
			}
			else 
			{
				if (((symb >= 'a') && (symb <= 'z')) || ((symb >= 'A') && (symb <= 'Z')))
				{
					lexem += symb;
					Sta = _C_;
					str_index++;
				}
				else
				{
					Sta = _F_;
				}
			}
			break;
		case _C_:
			if (len == str_index)
			{
				Sta = _F_;
			}
			else
			{
					if (((symb >= 'a') && (symb <= 'z')) || ((symb >= 'A') && (symb <= 'Z')) || ((symb >= '0') && (symb <= '9')) || (symb=='_'))
					{
						Sta = _C_;
						lexem += symb;
						str_index++;
					}
					else
					{
						Sta = _F_;
					}
				
			}
			break;
		}
	}
	if ((lexem == ""))
	{
		lastlexem = lexem;
		return false;
	}
	else 
	{
		lex = lexem;
		lastlexem = lexem;
		return true;
	}
}

bool ungetlex()
{
	if (lastlexem!="") 
	{
		is_unget = true;
		return true;
	}
	else{
		is_unget = false;
		return false;
	}
		
}
