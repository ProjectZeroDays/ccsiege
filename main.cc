#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
namespace pt = boost::property_tree;

#define VERSION 0.1

// we need some generic string to lower functions for later
void cstrtolower(char *cstring)
{
  for (int i=0; i<strlen(cstring); i++)
  {
    cstring[i] = tolower(cstring[i]);
  }
}

void cstrtolower(string *cstring)
{
  for (int i=0; i<cstring->size(); i++)
  {
    (*cstring)[i] = tolower((*cstring)[i]);
  }
}

// also convert to digit
int to_digit(char c)
{
  return c - '0';
}

// we also need a class to hold our app args
class arguments
{
  public:
  bool random;
  bool verify;
  bool version;
  bool walk;
  arguments();
  void set(string);
};

arguments::arguments()
{
  random = true;
  verify = false;
  walk = false;
  version = false;
}

void arguments::set(string key)
{
  if (key == "version")
  {
    version = true;
    return;
  }
  random = false;
  verify = false;
  walk = false;
  if (key == "random")
  {
    random = true;
  }
  else if (key == "verify")
  {
    verify = true;
  }
  else if (key == "walk")
  {
    walk = true;
  }
}

// ...and a class to hold IIN data for later
class iin
{
  public:
  long prefix;
  long length;
  iin(long, long);
};

iin::iin(long p, long l)
{
  prefix = p;
  length = l;
}

vector<iin> prefixes;

// calculate and return our check digit using luhn algo
int get_check_digit(string number)
{
  int sum = 0;
  for (int i = number.size()-1; i >= 0; i-=2)
  {
    // double the reverse odds and calculate the sum of their digits
    int x = to_digit(number[i]) * 2;
    while (x > 9)
    {
      x -= 9;
    }
    // add this to the total sum
    sum += x;

    // reverse evens are simply added to the total sum
    if (i-1 >=0)
    {
      sum += to_digit(number[i-1]);
    }
  }

  return ((sum / 10 + 1) * 10 - sum) % 10;
}

// generate a random PAN for a given prefix
void rand_pan(int prefix, int length)
{
  string pan;
  pan = to_string(prefix);
  while (pan.size() < length-1)
  {
    pan.append(to_string(rand() % 10));
  }

  cout << pan << get_check_digit(pan) << endl;
}

// generate all valid PANs for a given prefix
void walk_pan(int prefix, int length)
{
  string pan;
  int bucket(0);
  int prefix_size = to_string(prefix).size();
  int bucket_size = to_string(bucket).size();

  int counter = 1;
  while(bucket_size <= length - prefix_size - 1)
  {
    pan = to_string(prefix);
    if (length - prefix_size - bucket_size - 1)
    {
      pan.append(length - prefix_size - bucket_size - 1, '0');
    }
    pan.append(to_string(bucket));

    cout << pan << get_check_digit(pan) << endl;
    bucket++;
    bucket_size = to_string(bucket).size();
  }
}

// verify if a given PAN passes the Luhn algorithm
bool verify_pan(string pan)
{
  try
  {
    // make sure our input is a number
    stol(pan);
    // convert our check digit to an int
    int check_digit = to_digit(pan[pan.size()-1]);
    // validate check digit
    if (get_check_digit(pan.substr(0, pan.size()-1)) == check_digit)
    {
      return true;
    }
  }
  catch (invalid_argument& e)
  {
  }
  catch (out_of_range& e)
  {
    cerr << "Error! Invalid input." << endl;
  }
  return false;
}

int main(int argc, char** argv)
{
  arguments args;
  string iin_filename = "iin.txt";
  string opt;
  vector<string> countries;
  vector<string> issuers;
  vector<string> vendors;

  // parse options
  for ( int i=1; i<argc; i++ )
  {
    if (argv[i][0] != '-')
    {
      break;
    }
    opt = argv[i];
    if (opt == "-h" || opt == "--help")
    {
      cout << "usage: " << argv[0] << " [options]" << endl;
      cout << "    -c, --country     Specify a comma-separated list of 2-letter country codes to add IINs to the pool based on geographic location." << endl;
      cout << "    -d, --debug       Enable debug output" << endl;
      cout << "    -h, --help        Displays this help notice" << endl;
      cout << "    -i, --iin         Specify a list of IINs to use in the IIN pool. You can add an optional PAN length (colon-separated) for each IIN" << endl;
      cout << "    -I, --issuers     Specify a comma-separated list of issuers to use in the IIN pool" << endl;
      cout << "    -r, --random      Generate random PAN candidates for each IIN" << endl;
      cout << "        --verify      Checks input on stdin and outputs only the valid PANs" << endl;
      cout << "    -v, --version     Displays version info" << endl;
      cout << "    -V, --vendors     Specify a comma-separated list of vendors to use in the IIN pool" << endl;
      cout << "    -w, --walk        Generate all PAN candidates for each IIN" << endl;
      cout << endl;
      cout << "issuers:" << endl; 
      cout << "    Card issuers can be referenced by the issuer's name. Additionally, you can use shortnames with select issuers. Please reference the iin.txt file to view all entries. e.g." << endl;
      cout << "        amex" << endl;
      cout << "        dinersclub" << endl;
      cout << "        jcb" << endl;
      cout << "        mastercard" << endl;
      cout << "        visa" << endl;
      return 1;
    }
    else if (opt == "-c" || opt == "--country")
    {
      // split the argument into tokens
      cstrtolower(argv[++i]);
      boost::split(countries, argv[i], boost::is_any_of(","));
    }
    else if (opt == "-i" || opt == "--iin")
    {
      // split the argument into tokens
      vector<string> iin_list;
      boost::split(iin_list, argv[++i], boost::is_any_of(","));
      // now tokenize each entry into IIN and length
      for (int ii=0; ii < iin_list.size(); ii++)
      {
	vector<string> iin_meta;
	boost::split(iin_meta, iin_list[ii], boost::is_any_of(":"));
	// default length is none specified
	if (iin_meta.size() == 1)
	{
	  iin_meta.push_back("13");
	}

	// push IINs to pool
	prefixes.push_back(iin(stol(iin_meta[0]), stol(iin_meta[1])));
      }
    }
    else if (opt == "-I" || opt == "--issuers")
    {
      // split the argument into tokens
      cstrtolower(argv[++i]);
      boost::split(issuers, argv[i], boost::is_any_of(","));

      // expand issuer short names
      for (int i=0; i< issuers.size(); i++)
      {
	string issuer = issuers[i];
	cstrtolower(&issuer);
	if (issuer == "amex")
	{
	  issuers[i] = "american express";
	}
	else if (issuer == "diners")
	{
	  issuers[i] = "diners club";
	}
      }
    }
    else if (opt == "-V" || opt == "--vendors")
    {
      // split the argument into tokens
      cstrtolower(argv[++i]);
      boost::split(vendors, argv[i], boost::is_any_of(","));
    }
    else if (opt == "--verify")
    {
      args.set("verify");
    }
    else if (opt == "-v" || opt == "--version")
    {
      args.set("version");
    }
    else if (opt == "-w" || opt == "--walk")
    {
      args.set("walk");
    }

  }

  if (args.version)
  {
    cout << "ccsiege v" << VERSION << endl;
    cout << "written by unix-ninja" << endl;
    cout << "(please use this software responsibly)" << endl;
    return 1;
  }

  if (args.verify)
  {
    string line_input;
    while (cin >> line_input)
    {
      if (verify_pan(line_input))
      {	
	cout << line_input << endl;
      }
    }
    return 0;
  }

  // seed PRNG for generating PANs (does not need to be crypto safe)
  srand(time(NULL));

  if (!prefixes.size() || vendors.size() || issuers.size())
  {
    // verify iin file
    FILE *fp = fopen(iin_filename.c_str(),"r");
    if(!fp)
    {
      cerr << "Error! Unable to open IIN prefix file (" << iin_filename << ")." << endl;
      return 2;
    }
    fclose(fp);

    // seed the prefix pool
    pt::ptree json;
    try
    {
    pt::read_json(iin_filename, json);
    }
    catch (pt::json_parser::json_parser_error e)
    {
    }
    if (json.empty())
    {
      cerr << "Error! Unable to parse IIN prefix file (" << iin_filename << ")." << endl;
      return 2;
    }
    
    // push prefixes to pool
    for (pt::ptree::iterator it = json.begin(); it != json.end(); it++)
    {
      bool add_iin = true;

      // filter by vendor
      if (vendors.size())
      {
	string current_vendor = it->second.get<string>("vendor", "");
	cstrtolower(&current_vendor);
        if (find(vendors.begin(), vendors.end(), current_vendor) == vendors.end())
	{
	  add_iin = false;
	}
      }

      // filter by issuer
      if (issuers.size())
      {
	string current_issuer = it->second.get<string>("issuer", "");
	cstrtolower(&current_issuer);
        if (find(issuers.begin(), issuers.end(), current_issuer) == issuers.end())
	{
	  add_iin = false;
	}
      }

      // filter by country
      if (countries.size())
      {
	string current_country = it->second.get<string>("country", "");
	cstrtolower(&current_country);
        if (find(countries.begin(), countries.end(), current_country) == countries.end())
	{
	  add_iin = false;
	}
      }
      if (add_iin)
      {
        prefixes.push_back(iin(it->second.get<int>("iin"), it->second.get<int>("length", 13)));
      }
    }
  }

  // loop through PAN generation
  for (int i=0; i< prefixes.size(); i++)
  {
    if (args.random)
    {
      rand_pan(prefixes[i].prefix, prefixes[i].length);
    }
    if (args.walk)
    {
      walk_pan(prefixes[i].prefix, prefixes[i].length);
    }
  }

  return 0;
}
