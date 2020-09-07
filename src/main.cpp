#include <stdint.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "PlatformFile.h"

typedef struct WordExpress_
{
	std::vector<std::string> vecWord;
}SWordExpress;

std::string ParseChineseSymbol(const char *source, int &chineseLen)
{
	chineseLen = 0;
	std::string sChinese;
	size_t length = strlen(source);
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
			cur += 1;
		}
	}

	return sChinese;
}

void ExpressFromCsv(SWordExpress *h, const char *fileName)
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

			if ((chineseLen >= 3) && (chineseLen <= 10))
			{
				if (word.length() > 0)
				{
					h->vecWord.push_back(word);
				}
			}

			p = strtok(NULL, ",");
		}

		delete[]line;
	}

	fin.close();
}

void ExpressFromFile(SWordExpress *h, const SFileInfo *fileInfo)
{
	const SFileInfo *cur = fileInfo;
	while (cur)
	{
		if (cur->child)
		{
			ExpressFromFile(h, cur->child);
		}

		if (FILE_MODE_NORMAL_FILE == cur->mode)
		{
			if (strstr(cur->name, ".csv"))
			{
				printf("%s\n", cur->name);
				ExpressFromCsv(h, cur->name);
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
	if (argc < 2)
	{
		ShowUsage(argv[0]);
		return 1;
	}

	std::string path = argv[1];
	SFileInfo *fileInfo = GetFileInfo(path.c_str());

	SWordExpress *h = new SWordExpress();
	ExpressFromFile(h, fileInfo);

	std::ofstream fout;
	fout.open("words.csv", std::ios::out);
	if (h->vecWord.size() > 0)
	{
		std::sort(h->vecWord.begin(), h->vecWord.end());
		h->vecWord.erase(std::unique(h->vecWord.begin(), h->vecWord.end()), h->vecWord.end());

		// add BOM
		char cBom[] = { (char)0xEF, (char)0xBB, (char)0xBF, (char)0x00 };
		std::string sBom = cBom;
		fout << sBom;

		for (size_t i = 0; i < h->vecWord.size(); ++i)
		{
			fout << h->vecWord[i] << std::endl;
		}
	}
	delete h;

	FreeFileInfo(&fileInfo);
	return 0;
}