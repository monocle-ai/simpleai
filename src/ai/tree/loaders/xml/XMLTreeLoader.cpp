#include "XMLTreeLoader.h"
#include <tinyxml2.h>
#include "conditions/ConditionParser.h"
#include "tree/TreeNodeParser.h"

namespace ai {

XMLTreeLoader::XMLTreeLoader(const IAIFactory& aiFactory) :
		ITreeLoader(aiFactory) {
}

XMLTreeLoader::~XMLTreeLoader() {
}

bool XMLTreeLoader::init(const std::string& xmlData) {
	tinyxml2::XMLDocument doc(false);
	const int status = doc.Parse(xmlData.c_str());
	TreeNodeParser p;
	/** @todo implement parsing */
	return status == tinyxml2::XML_NO_ERROR;
}

}