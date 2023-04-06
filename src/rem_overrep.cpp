#include "rem_overrep.h"

std::pair<std::vector<std::string>, std::vector<std::string>> get_overrep_seqs_pe(SRA sra) {
  std::vector<std::string> overrepSeqs1;
  std::vector<std::string> overrepSeqs2;

  std::string inFile1Str(std::string(sra.get_fastqc_dir_2().first.c_str()) + ".filt_fastqc.html");
  std::string inFile2Str(std::string(sra.get_fastqc_dir_2().second.c_str()) + ".filt_fastqc.html");

  std::ifstream inFile1(inFile1Str);
  std::ifstream inFile2(inFile2Str);

  boost::regex rgx("(?<=<tr><td>)[ATCG]*(?=\<\/td>)");
  boost::smatch res;

  std::string inFile1Data;
  while (getline(inFile1, inFile1Data));
  std::string inFile2Data;
  while (getline(inFile2, inFile2Data));

  inFile1.close();
  inFile2.close();

  std::string::const_iterator sStart, sEnd;

  sStart = inFile1Data.begin();
  sEnd = inFile1Data.end();

  while (boost::regex_search(sStart, sEnd, res, rgx)) {
    overrepSeqs1.push_back(res.str());
    sStart = res.suffix().first;
  }

  sStart = inFile2Data.begin();
  sEnd = inFile2Data.end();

  while (boost::regex_search(sStart, sEnd, res, rgx)) {
    overrepSeqs2.push_back(res.str());
    sStart = res.suffix().first;
  }

  std::pair<std::vector<std::string>, std::vector<std::string>> overrepSeqs(overrepSeqs1, overrepSeqs2);  

  return overrepSeqs;
}


std::vector<std::string> get_overrep_seqs_se(SRA sra) {
  std::vector<std::string> overrepSeqs;

  std::string inFileStr(std::string(sra.get_fastqc_dir_2().first.c_str()) + "_fastqc.html");

  std::ifstream inFile(inFileStr);

  boost::regex rgx("(?<=<tr><td>)[ATCG]*(?=\<\/td>)");
  boost::smatch res;

  std::string inFileData;
  while (getline(inFile, inFileData));

  inFile.close();

  std::string::const_iterator sStart, sEnd;

  sStart = inFileData.begin();
  sEnd = inFileData.end();

  while (boost::regex_search(sStart, sEnd, res, rgx)) {
    overrepSeqs.push_back(res.str());
    sStart = res.suffix().first;
  }

  return overrepSeqs;
}


void rem_overrep_pe(std::pair<std::string, std::string> sraRunIn,
                    std::pair<std::string, std::string> sraRunOut,
                    uintmax_t ram_b,
                    std::pair<std::vector<std::string>, std::vector<std::string>> overrepSeqs) {
  std::string inFile1Str(sraRunIn.first);
  std::string inFile2Str(sraRunIn.second);
  std::string outFile1Str(sraRunOut.first);
  std::string outFile2Str(sraRunOut.second);
  std::ifstream inFile1(inFile1Str);
  std::ifstream inFile2(inFile2Str);
  std::ofstream outFile1(outFile1Str);
  std::ofstream outFile2(outFile2Str);

  if (overrepSeqs.first.empty() && overrepSeqs.second.empty()) {
    return;
  }
  int lenOverReps1;
  int lenOverReps2;
  if (!overrepSeqs.first.empty()) {
    lenOverReps1 = overrepSeqs.first.front().size();
  }
  if (!overrepSeqs.second.empty()) {
    lenOverReps2 = overrepSeqs.second.front().size();
  }

  uintmax_t ram_b_per_file = ram_b / 2;

  std::string inFile1Data;
  std::string inFile2Data;

  inFile1Data.reserve(ram_b_per_file);
  inFile2Data.reserve(ram_b_per_file);

  std::streamsize s1;
  std::streamsize s2;

  char * nlPos1;
  char * nlPos2;
  char * nlPos1Head;
  char * nlPos2Head;
  char * nlPos1Prev;
  char * nlPos2Prev;
  char * writeStart1;
  char * writeStart2;
  char * writeEnd1;
  char * writeEnd2;
  char * inFile1L;
  char * inFile2L;
  int seqLength;
  bool overRep;
 
  while (!inFile1.eof() || !inFile2.eof()) {
    inFile1.read(&inFile1Data[0], ram_b_per_file);
    inFile2.read(&inFile2Data[0], ram_b_per_file);

    s1 = inFile1.gcount();
    s2 = inFile2.gcount();

    nlPos1 = &inFile1Data[0];
    nlPos2 = &inFile2Data[0];
    writeStart1 = &inFile1Data[0];
    writeStart2 = &inFile2Data[0];
    
    inFile1L = &inFile1Data[0] + s1;
    inFile2L = &inFile2Data[0] + s2;

    align_file_buffer(inFile1, inFile2, &inFile1Data[0], &inFile2Data[0], s1, s2);

    while (nlPos1 != inFile1L && nlPos2 != inFile2L) {
      nlPos1Prev = nlPos1;
      nlPos2Prev = nlPos2;
      nlPos1Head = std::find(nlPos1 + 1, inFile1L, '\n');
      nlPos2Head = std::find(nlPos2 + 1, inFile2L, '\n');
      nlPos1 = std::find(nlPos1Head + 1, inFile1L, '\n');
      nlPos2 = std::find(nlPos2Head + 1, inFile2L, '\n');
      seqLength = nlPos1 - nlPos1Head - 1;
      overRep = false;
      if (!overrepSeqs.first.empty()) {
        for (int i = 0; i < seqLength - lenOverReps1; i++) {
          for (auto const & seq : overrepSeqs.first) {
            
            if (strncmp(nlPos1Head + 1 + i, &seq[0], lenOverReps1) == 0) {
              overRep = true;
              goto checkOverrep;
            }
          }
        }
      }
      if (!overrepSeqs.second.empty()) {
        for (int i = 0; i < seqLength - lenOverReps2; i++) {
          for (auto const & seq : overrepSeqs.second) {
            if (strncmp(nlPos2Head + 1 + i, &seq[0], lenOverReps2) == 0) {
              overRep = true;
              goto checkOverrep;
            }
          }
        }
      }
      checkOverrep:if (overRep) {
        writeEnd1 = nlPos1Prev;
        outFile1.write(writeStart1, writeEnd1 - writeStart1);
        writeEnd2 = nlPos2Prev;
        outFile2.write(writeStart2, writeEnd2 - writeStart2);
        
        for (int i = 0; i < 2; i++) {
          nlPos1 = std::find(nlPos1 + 1, inFile1L, '\n');
          nlPos2 = std::find(nlPos2 + 1, inFile2L, '\n');
        }
        writeStart1 = nlPos1;
        writeStart2 = nlPos2;
      }
      else {
        for (int i = 0; i < 2; i++) {
          nlPos1 = std::find(nlPos1 + 1, inFile1L, '\n');
          nlPos2 = std::find(nlPos2 + 1, inFile2L, '\n');
        }
      }
    }
    outFile1.write(writeStart1, &inFile1Data[0] + s1 - writeStart1);
    outFile2.write(writeStart2, &inFile2Data[0] + s1 - writeStart2);
  }
  inFile1.close();
  inFile2.close();
  outFile1.close();
  outFile2.close();
}


void rem_overrep_se(std::string sraRunIn, std::string sraRunOut,
                    uintmax_t ram_b,
                    std::vector<std::string> overrepSeqs) {
  std::ifstream inFile(sraRunIn);
  std::ofstream outFile(sraRunOut);

  int lenOverReps = overrepSeqs.front().size();

  std::string inFileData;

  inFileData.reserve(ram_b);

  std::streamsize s;

  char * nlPos;
  char * nlPosHead;
  char * nlPosPrev;
  char * writeStart;
  char * writeEnd;
  char * inFileL;
  int seqLength;
  bool overRep;

  while (!inFile.eof()) {
    inFile.read(&inFileData[0], ram_b);
    
    s = inFile.gcount();

    nlPos = &inFileData[0];
    writeStart = &inFileData[0];

    inFileL = &inFileData[0] + s;

    while (nlPos != inFileL) {
      nlPosPrev = nlPos;
      nlPosHead = std::find(nlPos + 1, inFileL, '\n');
      nlPos = std::find(nlPosHead + 1, inFileL, '\n');
      seqLength = nlPos - nlPosHead - 1;
      overRep = false;
      for (int i = 0; i < seqLength - lenOverReps; i++) {
        for (auto const & seq : overrepSeqs) {
          if (strncmp(nlPosHead + 1 + i, &seq[0], lenOverReps) == 0) {
            overRep = true;
            goto checkOverrep;
          }
        }
      }
      checkOverrep:if (overRep) {
        writeEnd = nlPosPrev;
        outFile.write(writeStart, writeEnd - writeStart);

        for (int i = 0; i < 2; i++) {
          nlPos = std::find(nlPos + 1, inFileL, '\n');
        }
        writeStart = nlPos;
      }
      else {
        for (int i = 0; i < 2; i++) {
          nlPos = std::find(nlPos + 1, inFileL, '\n');
        }
      }
    }
    outFile.write(writeStart, &inFileData[0] + s - writeStart);
  }
  inFile.close();
  outFile.close();
}


/*void rem_overrep_bulk(std::vector<std::pair<std::string, std::string>> sraRunsIn,
                      std::string ram_gb, std::string logFile) {
  uintmax_t ram_b = (uintmax_t)stoi(ram_gb) * 1000000000;
  for (auto sraRun : sraRunsIn) {
    // Check for checkpoint
    if (sra.checkpointExists("orep.fix")) {
      logOutput("Fixed version found for: ", logFile);
      summarize_sing_sra(sra, logFile, 2);
      continue;
    }
    //logOutput("\nNow processing:\n", logFile);
    if (sra.is_paired()) {
      //summarize_sing_sra(sra, logFile, 2);
      std::pair<std::vector<std::string>, std::vector<std::string>> overrepSeqs = get_overrep_seqs_pe(sra);
      rem_overrep_pe(sra, ram_b, overrepSeqs);
    }
    else {
      summarize_sing_sra(sra, logFile, 2);
      std::vector<std::string> overrepSeqs = get_overrep_seqs_se(sra);
      rem_overrep_se(sra, ram_b, overrepSeqs);
    }
    sra.makeCheckpoint("orep.fix");
  }
}*/
