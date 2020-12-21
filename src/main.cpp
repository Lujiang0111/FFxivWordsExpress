#include <stdint.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <algorithm>
#include "PlatformFile.h"

std::string ParseChineseSymbol(const char *source, int &chineseLen)
{
	chineseLen = 0;
	std::string sChinese;
	size_t length = strlen(source);

	bool findStart = false;
	size_t cur = 0;
	while (cur < length)
	{
		if (0xF0 == (source[cur] & 0xF0))
		{
			cur += 4;
		}
		else if (0xE0 == (source[cur] & 0xE0))
		{
			// 溢出判断
			if (cur + 2 >= length)
			{
				break;
			}

			// 只有当state为WORD_STATE_START时才开始计数
			if (!findStart)
			{
				break;
			}

			// 三位的，判断是不是中文
			int code = 0;
			for (int i = 0; i < 3; ++i)
			{
				code = (code << 8) + (uint8_t)source[cur + i];
			}

			if ((code >= 0xE4B880) && (code <= 0xE9BFA6))
			{
				// 汉字
				// 判断范围为E4 B8 80 ~ E9 BF A6

				++chineseLen;
				for (int i = 0; i < 3; ++i)
				{
					sChinese += source[cur + i];
				}
			}
			else
			{
				// 其他的舍弃
				return "";
			}
			
			cur += 3;
		}
		else if (0xC0 == (source[cur] & 0xC0))
		{
			cur += 2;
		}
		else
		{
			// 以"作为开始和结束符
			if (0x22 == source[cur])
			{
				if (findStart)
				{
					return sChinese;
				}
				else
				{
					findStart = true;
				}
			}

			cur += 1;
		}
	}

	return "";
}

void ExpressFromCsv(std::set<std::string> &setWord, const char *fileName)
{
	std::ifstream fin;
	fin.open(fileName, std::ios::in);

	std::string sline;
	while (std::getline(fin, sline))
	{
		char *line = new char[sline.length() + 1];

		// 去除BOM
		if ((sline.length() > 3) && (0xEF == (uint8_t)sline[0]) && (0xBB == (uint8_t)sline[1]) && (0xBF == (uint8_t)sline[2]))
		{
			strcpy(line, sline.c_str() + 3);
		}
		else
		{
			strcpy(line, sline.c_str());
		}

		char *p = strtok(line, ",");
		while (p)
		{
			int chineseLen = 0;
			std::string word = ParseChineseSymbol(p, chineseLen);

			if ((chineseLen >= 3) && (chineseLen <= 8))
			{
				if (word.length() > 0)
				{
					setWord.insert(word);
				}
			}

			p = strtok(NULL, ",");
		}

		delete[]line;
	}

	fin.close();
}

void ExpressFromFile(std::set<std::string> &setWord, const SPlatformFileInfo *fileInfo)
{
	const SPlatformFileInfo *cur = fileInfo;
	while (cur)
	{
		if (cur->child)
		{
			ExpressFromFile(setWord, cur->child);
		}

		if (PF_MODE_NORMAL_FILE == cur->mode)
		{
			if (strstr(cur->name, ".csv"))
			{
				printf("%s\n", cur->fullName);
				ExpressFromCsv(setWord, cur->fullName);
			}
		}

		cur = cur->next;
	}
}

static void ShowUsage(const std::string &progName)
{
	std::cerr << "Usage: " << progName << " path" << std::endl;
}

int main(int argc, char* argv[])
{
	std::string path;
	if (argc < 2)
	{
		std::cout << "Input csv root path: ";
		std::cin >> path;
	}
	else
	{
		path = argv[1];
	}

	SPlatformFileInfo *fileInfo = GetFileInfo(path.c_str(), PF_SORT_MODE_NONE);

	std::set<std::string> setWord;
	ExpressFromFile(setWord, fileInfo);

	std::ofstream fout;
	fout.open("words.csv", std::ios::out);
	if (setWord.size() > 0)
	{
		// add BOM
		char cBom[] = { (char)0xEF, (char)0xBB, (char)0xBF, (char)0x00 };
		std::string sBom = cBom;
		fout << sBom;

		for (const auto &x : setWord)
		{
			fout << x << std::endl;
		}
	}

	FreeFileInfo(&fileInfo);
	return 0;
}