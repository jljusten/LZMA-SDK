// LzmaAlone.h

#include <stdafx.h>

#include <initguid.h>
#include <limits.h>

#include "Common/CommandLineParser.h"
#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/PropVariant.h"

#include "../../Common/FileStreams.h"

#include "../LZMA/LZMADecoder.h"
#include "../LZMA/LZMAEncoder.h"

using namespace NWindows;
using namespace NCommandLineParser;

namespace NKey {
enum Enum
{
  kHelp1 = 0,
  kHelp2,
  kDictionary,
  kFastBytes,
  kLitContext,
  kLitPos,
  kPosBits,
  kEOS
};
}

static const CSwitchForm kSwitchForms[] = 
  {
    { L"?",  NSwitchType::kSimple, false },
    { L"H",  NSwitchType::kSimple, false },
    { L"D", NSwitchType::kUnLimitedPostString, false, 1 },
    { L"FB", NSwitchType::kUnLimitedPostString, false, 1 },
    { L"LC", NSwitchType::kUnLimitedPostString, false, 1 },
    { L"LP", NSwitchType::kUnLimitedPostString, false, 1 },
    { L"PB", NSwitchType::kUnLimitedPostString, false, 1 },
    { L"EOS", NSwitchType::kSimple, false }
  };

static const int kNumSwitches = sizeof(kSwitchForms) / sizeof(kSwitchForms[0]);

static void PrintHelp()
{
  printf("\nUsage:  LZMA <e|d> inputFile outputFile [<switches>...]\n"
             "  e: encode file\n"
             "  d: decode file\n"
    "<Switches>\n"
    "  -d{N}:  set dictionary - [0,28], default: 23 (8MB)\n"
    "  -fb{N}: set number of fast bytes - [5, 255], default: 128\n"
    "  -lc{N}: set number of literal context bits - [0, 8], default: 3\n"
    "  -lp{N}: set number of literal pos bits - [0, 4], default: 0\n"
    "  -pb{N}: set number of pos bits - [0, 4], default: 2\n"
    "  -eos:   write End Of Stream marker\n"
    );
}

static void PrintHelpAndExit(const char *s)
{
  printf("\nError: %s\n\n", s);
  PrintHelp();
  throw -1;
}

static void IncorrectCommand()
{
  PrintHelpAndExit("Incorrect command");
}

static void WriteArgumentsToStringList(int numArguments, const char *arguments[], 
    UStringVector &strings)
{
  for(int i = 1; i < numArguments; i++)
    strings.Add(MultiByteToUnicodeString(arguments[i]));
}

static bool GetNumber(const wchar_t *s, UINT32 &value)
{
  value  =0;
  if (wcslen(s) == 0)
    return false;
  const wchar_t *end;
  UINT64 res = ConvertStringToUINT64(s, &end);
  if (*end != L'\0')
    return false;
  if (res > 0xFFFFFFFF)
    return false;
  value = UINT32(res);
  return true;
}

int __cdecl main2(int n, const char *args[])
{
  printf("\nLZMA 4.00 Copyright (c) 2001-2004 Igor Pavlov  2004-02-13\n");

  if (n == 1)
  {
    PrintHelp();
    return -1;
  }

  UStringVector commandStrings;
  WriteArgumentsToStringList(n, args, commandStrings);
  CParser parser(kNumSwitches);
  try
  {
    parser.ParseStrings(kSwitchForms, commandStrings);
  }
  catch(...) 
  {
    IncorrectCommand();
  }

  if(parser[NKey::kHelp1].ThereIs || parser[NKey::kHelp2].ThereIs)
  {
    PrintHelp();
    return 0;
  }
  const UStringVector &nonSwitchStrings = parser.NonSwitchStrings;

  if(nonSwitchStrings.Size() != 3)  
    IncorrectCommand();

  bool encodeMode = false;
  const UString &command = nonSwitchStrings[0]; 
  const UString &inputName = nonSwitchStrings[1]; 
  const UString &outputName = nonSwitchStrings[2]; 

  if (command.CompareNoCase(L"e") == 0)
    encodeMode = true;
  else if (command.CompareNoCase(L"d") == 0)
    encodeMode = false;
  else
    IncorrectCommand();

  CInFileStream *inStreamSpec = new CInFileStream;
  CMyComPtr<IInStream> inStream = inStreamSpec;
  if (!inStreamSpec->Open(GetSystemString(inputName)))
  {
    printf("can not open input file %s", GetSystemString(inputName, CP_OEMCP));
    return 1;
  }

  COutFileStream *outStreamSpec = new COutFileStream;
  CMyComPtr<IOutStream> outStream = outStreamSpec;
  // boutStreamSpec->m_File.SetOpenCreationDispositionCreateAlways();
  if (!outStreamSpec->Open(GetSystemString(outputName)))
  {
    printf("can not open output file %s", GetSystemString(outputName, CP_OEMCP));
    return 1;
  }

  UINT64 fileSize;
  if (encodeMode)
  {
    NCompress::NLZMA::CEncoder *encoderSpec = 
      new NCompress::NLZMA::CEncoder;
    CMyComPtr<ICompressCoder> encoder = encoderSpec;

    UINT32 dictionary = 1 << 23;
    UINT32 posStateBits = 2;
    UINT32 litContextBits = 3; // for normal files
    // UINT32 litContextBits = 0; // for 32-bit data
    UINT32 litPosBits = 0;
    // UINT32 litPosBits = UINT32(2); // for 32-bit data
    UINT32 algorithm = 2;
    UINT32 numFastBytes = 128;

    bool eos = parser[NKey::kEOS].ThereIs;
    if(eos)
      encoderSpec->SetWriteEndMarkerMode(true);
 
    if(parser[NKey::kDictionary].ThereIs)
    {
      UINT32 dicLog;
      if (!GetNumber(parser[NKey::kDictionary].PostStrings[0], dicLog))
        IncorrectCommand();
      dictionary = 1 << dicLog;
    }
    if(parser[NKey::kFastBytes].ThereIs)
      if (!GetNumber(parser[NKey::kFastBytes].PostStrings[0], numFastBytes))
        IncorrectCommand();

    if(parser[NKey::kLitContext].ThereIs)
      if (!GetNumber(parser[NKey::kLitContext].PostStrings[0], litContextBits))
        IncorrectCommand();
    if(parser[NKey::kLitPos].ThereIs)
      if (!GetNumber(parser[NKey::kLitPos].PostStrings[0], litPosBits))
        IncorrectCommand();
    if(parser[NKey::kPosBits].ThereIs)
      if (!GetNumber(parser[NKey::kPosBits].PostStrings[0], posStateBits))
        IncorrectCommand();


    PROPID propIDs[] = 
    {
      NCoderPropID::kDictionarySize,
      NCoderPropID::kPosStateBits,
      NCoderPropID::kLitContextBits,
      NCoderPropID::kLitPosBits,
      NCoderPropID::kAlgorithm,
      NCoderPropID::kNumFastBytes
    };
    const int kNumProps = sizeof(propIDs) / sizeof(propIDs[0]);
    NWindows::NCOM::CPropVariant properties[kNumProps];
    properties[0] = UINT32(dictionary);
    properties[1] = UINT32(posStateBits);
    properties[2] = UINT32(litContextBits);
   
    properties[3] = UINT32(litPosBits);
    properties[4] = UINT32(algorithm);
    properties[5] = UINT32(numFastBytes);

    if (encoderSpec->SetCoderProperties(propIDs, properties, kNumProps) != S_OK)
      IncorrectCommand();
    encoderSpec->WriteCoderProperties(outStream);

    if (eos)
      fileSize = (UINT64)(INT64)-1;
    else
      inStreamSpec->File.GetLength(fileSize);
    outStream->Write(&fileSize, sizeof(fileSize), 0);

    if (encoder->Code(inStream, outStream, 0, 0, 0) != S_OK)
    {
      printf("Decoder error");
      return 1;
    }   

  }
  else
  {
    NCompress::NLZMA::CDecoder *decoderSpec = 
        new NCompress::NLZMA::CDecoder;
    CMyComPtr<ICompressCoder> decoder = decoderSpec;
    if (decoderSpec->SetDecoderProperties(inStream) != S_OK)
    {
      printf("SetDecoderProperties error");
      return 1;
    }
    UINT32 processedSize;
    if (inStream->Read(&fileSize, sizeof(fileSize), &processedSize) != S_OK)
    {
      printf("SetDecoderProperties error");
      return 1;
    }   
    
    // CComObjectNoLock<CDummyOutStream> *outStreamSpec = new CComObjectNoLock<CDummyOutStream>;
    // CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;

    for (int n = 0 ; n < 1; n++)
    {
      inStream->Seek(13, STREAM_SEEK_SET, 0);
      if (decoder->Code(inStream, outStream, 0, &fileSize, 0) != S_OK)
      {
        printf("Decoder error");
        return 1;
      }   
    }
  }
  return 0;
}


int __cdecl main(int n, const char *args[])
{
  try { return main2(n, args); }
  catch(...) { return -1; }
}
