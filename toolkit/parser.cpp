#include "parser.h"

Parser::Parser(const char *_delimiter) {
	if(_delimiter) {
		bzero(delimiter, sizeof(delimiter));
		strncpy(delimiter, _delimiter, sizeof(delimiter)-1);
		delimiterLenght = strlen(delimiter);
		parserInitialized = 1;
	}
	else {
		delimiter[0] = '\0';
		delimiterLenght = 1;
		parserInitialized = 0;
	}

	return;
}

Parser::Parser(int _minAsciiDelimiter, int _maxAsciiDelimiter) {
  if (_minAsciiDelimiter && _maxAsciiDelimiter) {
    minAsciiDelimiter = _minAsciiDelimiter;
    maxAsciiDelimiter = _maxAsciiDelimiter;
    parserInitialized = 2;
    delimiterLenght = 1;
  }
  else
    parserInitialized = 0;

  return;
}


Parser::~Parser() {
  return;
}

void Parser::upperCase(char *chaine) {
	int cpt = 0;

	if (! chaine)
		return;
	while (chaine[cpt])
	{
		chaine[cpt] = toupper(chaine[cpt]);
		cpt++;
	}

	return;
}

void Parser::deleteWhiteSpaces(String *memoryString) {
  int counter = 0;  
  int rotateCounter;
  int blocLenght = memoryString->getBlocSize();
  char *string = memoryString->getBloc();

  if (! memoryString) {
    systemLog->sysLog(LOG_ERR, "memoryString is NULL. cannot DELETE white spaces in this string\n");
    return;
  }
  while (string[counter] && (counter < blocLenght)) {
    if ((string[counter] == ' ') || (string[counter] == '\t')) {
      rotateCounter = counter;
      while ((rotateCounter+1 < blocLenght) && (string[rotateCounter+1])) {
        string[rotateCounter] = string[rotateCounter+1];
        rotateCounter++;
      }
      string[rotateCounter] = 0;
    }
    counter++;
  }
    
  return;
}

List<String *> *Parser::tokenizeString(char *chaine, int tokenizeLimit) {
	int cpt = 0;
	int cpt2;
	char *ptrdeb;
	String *argelt;
	List<String *> *tokenList;
	int len;

	if (! parserInitialized) {
		systemLog->sysLog(LOG_ERR, "parser parameters are not initialized correctly. cannot tokenize string\n");
		return NULL;
	}
	tokenList = new List<String *>();
	if (! tokenList) {
		systemLog->sysLog(CRITICAL, "cannot create a List<String *> object: %s\n", strerror(errno));
		return NULL;
	}
	tokenList->setDestroyData(2);
	if (! chaine)
		return NULL;
	ptrdeb = chaine;
	len = strlen(chaine);
//	while (chaine[cpt]) {
//		if ((chaine[cpt] == '\r') || (chaine[cpt] == '\n'))
//			chaine[cpt] = 0;
//		cpt++;
//	}
//	len = cpt;
	cpt = 0;
	while (cpt < len) {
		cpt2 = 0;
		if (parserInitialized == 1) {
			while ((cpt < len ) && strncmp(&chaine[cpt], delimiter, delimiterLenght)) {
				cpt++;
				cpt2++;
			}
		}
		else {
			while ((cpt < len ) && ! ((chaine[cpt] >= minAsciiDelimiter) && (chaine[cpt] <= maxAsciiDelimiter))) {
				cpt++;
				cpt2++;
			}
		}
		if (! cpt2)
			argelt = NEW String(1);
		else {
			argelt = NEW String(cpt2+1);
			strncpy(argelt->getBloc(), ptrdeb, argelt->getBlocSize());
			argelt->getBloc()[cpt2] = 0;
			tokenizeLimit--;
		}
		tokenList->addElement(argelt);
		cpt += delimiterLenght;
		if (parserInitialized == 2)
			ptrdeb = &chaine[cpt-delimiterLenght];
		else
			ptrdeb = &chaine[cpt];
		if (! tokenizeLimit) {
			argelt = NEW String(strlen(ptrdeb)+1);
			strncpy(argelt->getBloc(), ptrdeb, argelt->getBlocSize());
			tokenList->addElement(argelt);
			return tokenList;
		}
	}

	return tokenList;
}

List<Memory<char> *> *Parser::tokenizeData(char *chaine, int tokenizeLimit, ssize_t size) {
	ssize_t cpt = 0;  
	int cpt2;
	char *ptrdeb;
	Memory<char> *argelt;
	List<Memory<char> *> *tokenList;

	if (! parserInitialized) {
		systemLog->sysLog(ERROR, "parser parameters are not initialized correctly. cannot tokenize string\n");
		return NULL;
	}
	tokenList = new List<Memory<char> *>();
	if (! chaine)
		return NULL;
	ptrdeb = chaine;
	cpt = 0;
	while (cpt < size) {
		cpt2 = 0;
		if (parserInitialized == 1) {
			while ((cpt < size ) && memcmp(&chaine[cpt], delimiter, delimiterLenght)) {
				cpt++;
				cpt2++;
			}
		}
		else {
			while ((cpt < size ) && ! ((chaine[cpt] >= minAsciiDelimiter) && (chaine[cpt] <= maxAsciiDelimiter))) {
				cpt++;
				cpt2++;
			}
		}
		if (! cpt2)
			argelt = new Memory<char>(1);
		else {
			argelt = new Memory<char>(cpt2+1);
			memcpy(argelt->bloc, ptrdeb, cpt2);
			tokenizeLimit--;
		}
		tokenList->addElement(argelt);
		if (cpt >= size)
			break;
		cpt += delimiterLenght;
		if (parserInitialized == 2)
			ptrdeb = &chaine[cpt-delimiterLenght];
		else
			ptrdeb = &chaine[cpt];
		if (! tokenizeLimit) {
			argelt = new Memory<char>((chaine+size)-ptrdeb+1);
			memcpy(argelt->getBloc(), ptrdeb, (chaine+size)-ptrdeb);
			tokenList->addElement(argelt);
			return tokenList;
		}
	}

	return tokenList;
}
