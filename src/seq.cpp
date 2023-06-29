#include "seq.h"

sequence::sequence() {
  header = "";
  sequenceData = "";
  quality = "";
  int numBp = -1;
}

sequence::sequence(std::string header, std::string sequenceData) {
  this->header = header;
  size_t nlPos = sequenceData.find("\n");
  this->sequenceData = sequenceData;
  this->id = "";
  numBp = sequenceData.length();
}

sequence::sequence(std::string header, std::string sequenceData, std::string quality) {
  this->header = header;
  size_t nlPos = sequenceData.find('\n');
  this->sequenceData = sequenceData;
  this->quality = quality;
  this->id = "";
  numBp = sequenceData.length();
}

sequence::sequence(const sequence & seq) {
  header = seq.header;
  sequenceData = seq.sequenceData;
  quality = seq.quality;
  numBp = seq.numBp;
  id = seq.id;
}

std::string sequence::get_header() {
  return this->header;
}

std::string sequence::get_sequence() {
  return this->sequenceData;
}

std::string sequence::get_quality() {
  return this->quality;
}

std::string sequence::get_id() {
  return this->id;
}

void sequence::set_header(std::string newHeader) {
  this->header = newHeader;
}

void sequence::set_id(std::string newId) {
  this->id = newId;
}
