/*
Copyright (c) <2012>, <Georgia Institute of Technology> All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions
and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or other materials provided
with the distribution.

Neither the name of the <Georgia Institue of Technology> nor the names of its contributors
may be used to endorse or promote products derived from this software without specific prior
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

/**********************************************************************************************
 * File         : knob.cc
 * Author       : Abderrahim Benquassmi
 * Date         : 3/11/2008
 * CVS          : $Id: knob.cc 868 2009-11-05 06:28:01Z kacear $:
 * Description  : knob template
 *********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "knob.h"
#include "all_knobs.h"

namespace cxlsim {

// constructor
KnobsContainer::KnobsContainer() {
  m_allKnobs = new all_knobs_c();  // instantiates all knobs
  m_allKnobs->registerKnobs(this);  // registers all knobs into the container
}

// destructor
KnobsContainer::~KnobsContainer() {
  delete m_allKnobs;
}

// clear all knobs
void KnobsContainer::clear() {
}

// apply new values from the file
void KnobsContainer::applyParamFile(const std::string& filename) {
  std::ifstream file;
  file.open(filename.c_str(), std::ifstream::in);
  if (file.is_open()) {
    std::string line;
    while (!file.eof()) {
      getline(file, line);
      createEntryFromtext(line);
    }
    file.close();
  } else {
    std::cout << "Unable to open " << filename << std::endl;
  }
  applyValuesToKnobs(m_valuesFromFile);
}

// insert a knob to the table
void KnobsContainer::insertKnob(abstract_knob_c* pKnob) {
  m_theKnobs[pKnob->getName()] = pKnob;
}

// get the reference to all the knobs for this component instance
all_knobs_c* KnobsContainer::getAllKnobs() {
  return m_allKnobs;
}

// create a new entry from the table
void KnobsContainer::createEntryFromtext(const std::string& text) {
  bool success = true;
  param_file_entry_s entry;
  std::vector<std::string> tokens;

  m_theTokenizer.tokenizeString(text, ' ');
  m_theTokenizer.getTokens(tokens);

  if (tokens.size() < 2 || tokens[0].at(0) == MAGIC_COMMENTS) {
    success = false;
  }

  if (success) {
    entry.m_name = tokens[0];
    entry.m_value = tokens[1];
    m_valuesFromFile.insert(std::pair<std::string, std::string>(entry.m_name, entry.m_value));
  }

  m_theTokenizer.clear();
  return;
}

// dump out all knobs to the file
void KnobsContainer::saveToFile(const std::string& filename) {
  std::ofstream file;
  file.open(filename.c_str());
  if (file.is_open()) {
    file << (*this);
    file.close();
  }
}

// adjust knob values
void KnobsContainer::adjustKnobValues() {
  applyValuesToKnobs(m_valuesFromFile);
}

// update a single knob
void KnobsContainer::updateKnob(std::string key, std::string value) {
  createEntryFromtext(key + " " + value);
  adjustKnobValues();
}

// apply new value to the knob
void KnobsContainer::applyValuesToKnobs(
  std::map<std::string, std::string, ltstr_s>& ValuesMap) {
  if (ValuesMap.size() == 0) {
    return;
  }

  std::map<std::string, std::string, ltstr_s>::iterator iter = ValuesMap.begin();
  std::map<std::string, std::string, ltstr_s>::iterator end = ValuesMap.end();
  abstract_knob_c* pKnob = NULL;
  while (iter != end) {
    pKnob = m_theKnobs[iter->first];
    if (NULL != pKnob) {
      std::string valueString = iter->second;
      pKnob->initFromString(valueString);
    } else {
      std::cout << "Knob ' " << iter->first << " ' not found" << std::endl;
    }
    ++iter;
  }
  // apply values to dependent knobs
  std::map<std::string, abstract_knob_c*, ltstr_s>::iterator knob_iter =
    m_theKnobs.begin();
  std::map<std::string, abstract_knob_c*, ltstr_s>::iterator knob_end = m_theKnobs.end();
  while (knob_iter != knob_end) {
    pKnob = (abstract_knob_c*)(knob_iter->second);
    if (pKnob == NULL) {
      // do nothing
    } else if (!pKnob->getParentName().empty() &&
        m_theKnobs[pKnob->getParentName()]->wasValueProvided() &&
        !pKnob->wasValueProvided()) {
      pKnob->initFromString(
        m_theKnobs[pKnob->getParentName()]->getValueString());
    }
    ++knob_iter;
  }
}

// apply new values from commandline argument
bool KnobsContainer::applyComandLineArguments(int argc, char** argv,
                                              char** invalidParam) {
  std::vector<std::string> tokens;
  for (int i = 1; i < argc; ++i) {
    char* pArg = argv[i];
    if (strlen(pArg) < 3) {
      continue;
    }

    if ((pArg[0] == '-') && (pArg[1] == '-')) {
      std::string argument(pArg + 2);
      m_theTokenizer.clear();
      m_theTokenizer.tokenizeString(argument, '=');
      tokens.clear();
      m_theTokenizer.getTokens(tokens);
      if (tokens.size() < 2) {
        *invalidParam = pArg;
        std::cout << "Not enough fileds: -- " << argument << std::endl;
        return false;
      }
      std::string knobName = tokens[0];
      std::string value = tokens[1];
      if (knobName.size() && value.size()) {
        m_valuesFromCommandLineSwitches.insert(
          std::pair<std::string, std::string>(knobName, value));
      }
    } else {
      *invalidParam = pArg;
      std::cout << "Invalid or incomplete argument  ' " << pArg
           << " ' has been ignored" << std::endl;
    }
  }
  applyValuesToKnobs(m_valuesFromCommandLineSwitches);
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////

// return number of tokens
int KnobEntryTokenizer::numTokens() {
  int tokenNumber = m_TokensVector.size();
  return tokenNumber;
}

// tokenize a std::string with the delimiter
void KnobEntryTokenizer::tokenizeString(std::string str, char delim) {
  static const std::string::size_type npos = (std::string::size_type)-1;
  std::string::size_type index1, index2;
  index2 = 0;
  while ((index1 = str.find_first_not_of(delim, index2)) != npos) {
    index2 = str.find_first_of(delim, index1);
    std::string token = str.substr(index1, index2 - index1);
    if (token.size() > 0) {
      m_TokensVector.push_back(token);
    }
  }
}

// get tokens from m_TokensVector
void KnobEntryTokenizer::getTokens(std::vector<std::string>& array) {
  std::vector<std::string>::iterator iter = m_TokensVector.begin();
  std::vector<std::string>::iterator last = m_TokensVector.end();
  while (iter != last) {
    array.push_back(*iter);
    ++iter;
  }
}

} // namespace CXL
