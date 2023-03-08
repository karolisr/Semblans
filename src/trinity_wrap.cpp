#include "trinity_wrap.h"

// TODO: Change condition by which multi-SRA assembly is determined
//   NOW: SRAs with same organism name are combined
//     - Problem if local files, without organism names are used

std::vector<SRA> get_sra_to_combine(std::vector<SRA> sras, std::string org_name) {
  std::vector<SRA> sra_comb;
  for (auto &sra : sras) {
    if (sra.get_org_name() == org_name) {
      sra_comb.push_back(sra);
    }
  }
  return sra_comb;
}

std::string combine_reads(std::vector<SRA> sras_comb, long long int ram_b, std::string logFile) {
  std::string inFileStr1;
  std::string outDirStr(sras_comb[0].get_sra_path_orep_filt().first.parent_path().c_str());
  std::string outFileStr(outDirStr + "/" + sras_comb[0].get_tax_id() + "_" +
                         sras_comb[0].get_org_name().replace(sras_comb[0].get_org_name().find(" "), 1, "_") +
                         ".fasta");

  std::string inFileBuffer;
  inFileBuffer.reserve(ram_b);
  std::streamsize s;

  std::ofstream outFile;
  std::ifstream inFile;
  logOutput("Combined assembly chosen", logFile);
  logOutput("Now combining files for assembly with Trinity ...", logFile);
  if (fs::exists(outFileStr)) {
    logOutput("Combined FASTA file found for: " + sras_comb[0].get_org_name(), logFile);
    return outFileStr;
  }
  for (auto &sra : sras_comb) {
    inFile.open(sra.get_sra_path_orep_filt().first.c_str());
    outFile.open(outFileStr, std::ios_base::app);
    while (!inFile.eof()) {
      inFile.read(&inFileBuffer[0], ram_b);
      s = inFile.gcount();
      outFile.write(&inFileBuffer[0], s);
    }
    inFile.close();
    if (sra.is_paired()) {
      inFile.open(sra.get_sra_path_orep_filt().second.c_str());
      while (!inFile.eof()) {
        inFile.read(&inFileBuffer[0], ram_b);
        s = inFile.gcount();
        outFile.write(&inFileBuffer[0], s);
      }
      inFile.close();
    }
    outFile.close();
  }
  logOutput("Complete!\nNow initiating Trinity assembly ...", logFile);
  return outFileStr;
}

transcript run_trinity(SRA sra, std::string threads, std::string ram_gb,
                       bool dispOutput, std::string logFile) {
  // Run Trinity for assembly using single SRA
  std::string inFile1;
  std::string inFile2;
  transcript sra_trans(sra);
  std::string outFile(sra.get_accession() + "_" + sra_trans.get_trans_path_trinity().c_str());
  std::string trin_cmd;
  std::string printOut;
  if (dispOutput) {
    printOut = " 2>&1 | tee -a " + logFile;
  }
  else {
    printOut = " >>" + logFile + " 2>&1";
  }
  int result;
  if (sra.is_paired()) {
    trin_cmd = PATH_TRINITY + " --seqType fq" + " --left " +
               sra.get_sra_path_orep_filt().first.c_str() + " --right " +
               sra.get_sra_path_orep_filt().second.c_str() + " --max_memory " + ram_gb + "G " +
               "--CPU " + threads + " --bflyCalculateCPU" + " --full_cleanup" +
               " --no_normalize_reads" + " --output " + outFile + printOut;
  }
  else {
    trin_cmd = PATH_TRINITY + " --seqType fq" + " --single " +
               sra.get_sra_path_orep_filt().first.c_str() + " --max_memory " + ram_gb + "G " +
               "--CPU " + threads + " --bflyCalculateCPU" + " --full_cleanup" +
               " --no_normalize_reads" + " --output " + outFile + printOut;
  }
  result = system(trin_cmd.c_str());
  if (WIFSIGNALED(result)) {
    logOutput("Exited with signal " + std::to_string(WTERMSIG(result)), logFile);
    exit(1);
  }
  std::rename((outFile + ".Trinity.fasta").c_str(), outFile.c_str());
  return sra_trans;
}

transcript run_trinity_comb(std::vector<SRA> sras_comb,
                            std::string threads, std::string ram_gb,
                            bool dispOutput, std::string logFile) {
  // Run Trinity for assembly using multiple SRAS
  long long int ram_b = (long long int)stoi(ram_gb) * 1000000000;
  std::string inFile = combine_reads(sras_comb, ram_b, logFile);
  transcript sra_trans(sras_comb[0]);
  std::string outFile(sra_trans.get_trans_path_trinity().c_str());
  std::string trin_cmd;
  std::string printOut;
  if (dispOutput) {
    printOut = " 2>&1 | tee -a " + logFile;
  }
  else {
    printOut = " >>" + logFile + " 2>&1";
  }
  int result;
  trin_cmd = PATH_TRINITY + " --seqType fq" + " --single " + inFile + " --max_memory " +
             ram_gb + "G " + "--CPU " + threads + " --bflyCalculateCPU" + " --full_cleanup" +
             " --no_normalize_reads" + " --run_as_paired" + " --output " + outFile + printOut; 
  result = system(trin_cmd.c_str());
  if (WIFSIGNALED(result)) {
    logOutput("Exited with signal " + std::to_string(WTERMSIG(result)), logFile);
    exit(1);
  }
  std::rename((outFile + ".Trinity.fasta").c_str(), outFile.c_str());
  return sra_trans;
}

std::vector<transcript> run_trinity_bulk(std::vector<SRA> sras,
                                         std::string threads, std::string ram_gb,
                                         bool mult_sra, bool dispOutput, std::string logFile) {
  // Iterate through SRAs, running Trinity for all
  std::vector<transcript> sra_transcripts;
  transcript currSraTrans;
  for (auto &sra : sras) {
    if (fs::exists(transcript(sra).get_trans_path_trinity().c_str())) {
      logOutput("Assembly found for: ", logFile);
      summarize_sing_sra(sra, logFile);
      continue;
    }
    if (mult_sra) {
      std::vector<SRA> sras_comb = get_sra_to_combine(sras, sra.get_org_name());
      currSraTrans = run_trinity_comb(sras_comb, threads, ram_gb, dispOutput, logFile);
    }
    else {
      currSraTrans = run_trinity(sra, threads, ram_gb, dispOutput, logFile);
    }
    sra_transcripts.push_back(currSraTrans);
  }
  return sra_transcripts;
}
