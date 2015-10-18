//
// C++ Implementation: httpexchange
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <unistd.h>

#include "httpexchange.h"

HttpExchange::HttpExchange(int _outputDescriptor) {
	inputDescriptor = 0;
	outputDescriptor = _outputDescriptor;
	inputPtr = NULL;
	inputOffset = 0;
	inputPtrOffset = 0;
	mediaType = 0;

	return;
}


HttpExchange::~HttpExchange() {
	if (inputDescriptor)
		close(inputDescriptor);

	return;
}

int HttpExchange::getInput(void) {
	return inputDescriptor;
}

int HttpExchange::getOutput(void) {
	return outputDescriptor;
}

void HttpExchange::setInput(int descriptor) {
	inputDescriptor = descriptor;
	return;
}

char *HttpExchange::getInputPtr(void) {
	return inputPtr;
}

void HttpExchange::setInputPtr(char *ptr) {
	inputPtr = ptr;
	return;
}

int HttpExchange::getInputOffset(void) {
	return inputOffset;
}

void HttpExchange::setInputOffset(int offset) {
	inputOffset = offset;

	return;
}

int HttpExchange::getInputPtrOffset(void) {
	return inputPtrOffset;
}

void HttpExchange::setInputPtrOffset(int offset) {
	inputPtrOffset = offset;

	return;
}

int HttpExchange::getMediaType(void) {
	return mediaType;
}

void HttpExchange::setMediaType(int _mediaType) {
	mediaType = _mediaType;

	return;
}
