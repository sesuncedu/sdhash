// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "../gen-cpp/sdhashsrv.h"
#include <protocol/TBinaryProtocol.h>
#include <concurrency/ThreadManager.h>
#include <concurrency/PlatformThreadFactory.h>
#include <server/TSimpleServer.h>
#include <server/TThreadPoolServer.h>
#include <server/TThreadedServer.h>
#include <transport/TServerSocket.h>
#include <transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::concurrency;

using boost::shared_ptr;

using namespace  ::sdhash;

using namespace std;

#include <iostream>
#include <fstream>
#include <vector>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include "sdbf_class.h"
#include "sdbf_defines.h"
#include "sdbf_set.h"
#include "sdhash-srv.h"
#include "set_list.h"

#include <omp.h>

namespace fs = boost::filesystem;

//static pthread_mutex_t set_mutex = PTHREAD_MUTEX_INITIALIZER;


/** 
    Handler class for sdhash-srv 
    Contains implmementation of service calls
*/
class sdhashsrvHandler : virtual public sdhashsrvIf {
 public:

    /** Handler initialization, starts up server
        \param thread_count number of threads to use for processing
        \param home directory to search for hashsets
    */
  sdhashsrvHandler(uint32_t thread_count, std::string *home, std::string *sources) {
    sdbf::config = new sdbf_conf(thread_count, 0, _MAX_ELEM_COUNT, _MAX_ELEM_COUNT_DD);
    uint32_t j=0;
    source_directory=*sources; 
    home_directory=*home; 
    processing_thread_count=thread_count;
    rng_init();
    sdbf_set *tmp_set;
    if (fs::is_directory(home->c_str())) {
        for (fs::directory_iterator itr(home->c_str()); itr!=fs::directory_iterator(); ++itr) {
          if (fs::is_regular_file(itr->status())) {
               try {
                fprintf(stderr,"-> Loading file %s ..... ",itr->path().string().c_str());
                tmp_set=new sdbf_set(itr->path().string().c_str());
                if (!tmp_set->empty()) {
                    int32_t setid=make_hashsetID();
                    string setname;
                    setname=itr->path().stem().string();
                    //add_set(tmp_set,(char*)setname.c_str(),setid);
                    j++;
                    fprintf(stderr,"OK\n");
                if (fs::exists(itr->path().string()+".idx")) {
                bloom_filter *indextest=new bloom_filter(itr->path().string()+".idx");
                indexlist.push_back(indextest);
                tmp_set->index=indextest;
                        cerr << "-> Loaded index " <<  itr->path().string() << "..... OK" << endl;
            }
                    add_set(tmp_set,(char*)setname.c_str(),setid);
                } else {
                    fprintf(stderr,"File empty\n",itr->path().string().c_str());
                }
            } catch (int e) {
                if (e==-1) 
                    fprintf(stderr,"File not accessible\n");
                if (e==-2) 
                    fprintf(stderr,"File format invalid\n");
                if (e==-3) 
                    fprintf(stderr,"File too small\n");

            }
          }
        }
    }
 
    fprintf(stderr, "sdhash-srv loaded %d sets.\n",j );
  }

      /** 
        ping()    
  */
  void ping() {
    // Your implementation goes here
    printf("ping\n");
  }

  /** 
    retrieves listing of sets
    \param _return string of listing    
    \param json use json output if true
  */
  void setList(std::string& _return, const bool json) {
    int i; 
    int defchar='K';  
    uint64_t sizetmp=0;
    uint64_t count;
    map<int32_t,sdbf_set*>::iterator it;
    cout << "setList request" << endl;
    std::stringstream out;
    if (json) 
        out << "{ \"aaData\": [";
    i=0;
    for (it=setcollection.begin();it!=setcollection.end(); it++,i++) {
        sizetmp = (*it).second->input_size();
        count = (*it).second->size();
        if (sizetmp > GB)  {
            sizetmp = sizetmp/ GB;
            defchar = 'G';
        } else if (sizetmp > MB ) {
            sizetmp = sizetmp / MB;
            defchar = 'M';
        } else  {
            sizetmp = sizetmp / KB;
            defchar = 'K';
        }
        if (json) {
            if (i!=0)
                out << ",";
            out << "[\""<< (*it).first << "\",\"" ;
            out << (*it).second->name() <<"\",\"" ;
            out << sizetmp << (char) defchar << "\",\"" ;
            out << count << "\"]";
        } else {
            out << (*it).first << " " ;
            out << (*it).second->name() << " " ;
            out << sizetmp << (char) defchar << "\n" ;
        }
    }
    if (json) 
        out << "] }";
    if (get_set_count() == 0) {
        if (json)
            _return="[\"sdhash-srv ready, no sets loaded\"]";
        else 
            _return="sdhash-srv ready, no sets loaded\n";
    } else 
        _return=out.str();
  }

  /** 
    retrieves listing of set contents and sizes
    \param _return string of listing
    \param num1 number of set
  */
  void displaySet(std::string& _return, const int32_t num1) {
    std::stringstream out;
    uint64_t sizetmp=0;    
    int defchar='K';
    if (!setcollection.count(num1)) {
        std::cout << "displaySet request for " << num1 << " set not found." << std::endl;
        _return="Error: not found\n";
    } else {
        std::cout << "displaySet request for " << num1 << " " << get_set_name(num1) << std::endl;
        sdbf_set *target=get_set(num1);
        for (int i=0; i < target->size(); i++) {
            out << target->at(i)->name() << " ";
            sizetmp=target->at(i)->input_size();     
            if (sizetmp > GB)  {
                sizetmp = sizetmp/ GB;
                defchar = 'G';
            } else if (sizetmp > MB ) {
                sizetmp = sizetmp / MB;
                defchar = 'M';
            } else  {
                sizetmp = sizetmp / KB;
                defchar = 'K';
            }
            out << sizetmp << (char) defchar << "\n" ;
        }
        _return=out.str();
    }
  }

  /** 
    retrieves set hash contents 
    \param _return string of displayable hashes
    \param num1 number of set
  */
  void displayContents(std::string& _return, const int32_t num1) {
    std::stringstream out;
    if (!setcollection.count(num1)) {
        _return="Error: not found\n";
        std::cout << "displayContents request for " << num1 << " set not found." << std::endl;
    } else {
        std::cout << "displayContents request for " << num1 << " " << get_set_name(num1) << std::endl;
        out << get_set(num1);
        _return=out.str();
    }
  }

  /** 
    compares all hashes in passed set, associates them with ID when finished
    \param num1 number of set
    \param threshold output confidence threshold
    \param resultID ID to store results under
  */
  void compareAll(const int32_t num1, const int32_t threshold, const int32_t resultID) {
    std::string result_str;
    if (setcollection.count(num1)) {
        add_request(resultID,(string)get_set_name(num1)+" all hashes");
        std::cout << "compareAll begin request for " << num1 << " " << get_set_name(num1) <<  std::endl;
        result_str=get_set(num1)->compare_all_quiet(threshold,processing_thread_count);
        add_result(resultID,result_str);
    } else {
        std::cout << "compareAll begin request for " << num1 << " not found" << std::endl;
    }
    std::cout << "compareAll request ends for " << num1 << std::endl;
  }

  /** 
    compares two passed sets, associates them with ID when finished
    \param num1 number of set
    \param num2 number of set
    \param threshold output confidence threshold
    \param sample count of filters to sample in query
    \param resultID ID to store results under
  */
  void compareTwo(const int32_t num1, const int32_t num2, const int32_t threshold, const int32_t sample, const int32_t resultID) {
    std::string result_str;
    if ((setcollection.count(num1)) &&(setcollection.count(num2)))  {
        add_request(resultID,(string)get_set_name(num1)+" -- "
            +(string)get_set_name(num2));
        std::cout << "compareTwo begin request for " << num1 << " " << get_set_name(num1) ;
        std::cout << " "<< num2 << " " << get_set_name(num2) <<  std::endl;
        result_str=get_set(num1)->compare_to_quiet(get_set(num2),threshold,sample,processing_thread_count);
        add_result(resultID,result_str);
    } else {
        std::cout << "compareTwo begin request for " << num1 ;
        std::cout << ", "<< num2 << " sets not found" << std::endl;
    }
    std::cout << "compareTwo request ends for " << num1 ;
    std::cout << " "<< num2 <<  std::endl;
  }

  /** 
    loads a hashset from a file.  
    \param filename filename containing hash set
    \returns int32_t id, < 0 on failure
  */
  int32_t loadSet(const std::string& filename, const int32_t hashsetID) {
    sdbf_set *tmp_set;
    char *setname;
    setname=(char*)malloc(filename.length()+1);
    strncpy(setname,filename.c_str(),filename.length()+1);
    int32_t setid=-1;
    int fail=0;
    std::cout << "loadSet begin request to load " << filename << std::endl;
    try {
        tmp_set=new sdbf_set(filename.c_str());
    // load index, attach to set
    if (fs::exists(filename+".idx")) {
            bloom_filter *indextest=new bloom_filter(filename+".idx");
        indexlist.push_back(indextest);
            tmp_set->index=indextest;
    }
        if (!tmp_set->empty()) {
            setid=make_hashsetID();
            add_set(tmp_set,setname,setid);
        } else {
            fprintf(stderr,"File not found or empty: %s\n",filename.c_str());
            fail=1;
        }
    } catch (int e) {
        if (e==-1) 
            fprintf(stderr,"File not accessible: %s\n",filename.c_str());
        if (e==-2) 
            fprintf(stderr,"File format invalid: %s\n",filename.c_str());
        if (e==-3) 
            fprintf(stderr,"File too short: %s\n",filename.c_str());
        fail=1;
    }
    if (!fail)
        std::cout << "loadSet request successful for " << filename << std::endl;
    else
        std::cout << "loadSet request failed for " << filename << std::endl;
    return setid;
  }

  int32_t saveSet(const int32_t num1, const std::string& filename) {
    std::cout << "saveSet begin request for "<< num1 << " " << filename << std::endl;
    if (setcollection.count(num1)) {
        std::filebuf fb;
        fb.open (filename.c_str(),ios::out);
        if (fb.is_open()) {
            std::ostream os(&fb);
            os << get_set(num1);
            fb.close();
            std::cout << "saveSet request succeeded "<< num1 << " " << filename << std::endl;
        } else {
            fprintf(stderr,"saveSet cannot write to file %s\n",filename.c_str());
            return -1;
        }
    } else  {
        fprintf(stderr,"saveSet set number not valid %d\n",num1);
        return -1;
    }
    return 0;
  }

  int32_t createHashsetID() {
    printf("createHashsetID\n");
    return make_hashsetID();
  }

  void hashString(const std::string& setname, const std::vector<std::string> & filenames, const int32_t blocksize, const int32_t hashsetID, const int32_t searchIndex) {
    sdbf_set *tmp;
    index_info *info=(index_info*)malloc(sizeof(index_info));
    std::cout << "hashString begin request for "<< setname << " type " << searchIndex << endl;
    switch (searchIndex) {
        case 1:
            // search top
            info->search_first=true;
            info->search_deep=true;
        break;
        case 2: 
            // search all
            info->search_first=false;
            info->search_deep=true;
        break;
        case 3:
            // search set
            info->search_first=false;
            info->search_deep=false;
        break;
    }
    // if we are searching:
    if (searchIndex > 0 ) {
    tmp=new sdbf_set();
    info->indexlist=&indexlist;
    info->setlist=&setlist;
        info->index=NULL;    
        cerr << "Searching " << indexlist.size() << " indexes." << endl; 
        int resID=createResultID("indexing");
    add_request(resID,setname+" searching indexes");
        tmp=hash_stringlist(filenames,blocksize,processing_thread_count, info);
        string indexres=tmp->index_results();
    add_result(resID,indexres);
    delete tmp;
        std::cout << "hashString request succeeds for "<< setname << std::endl;
        // do NOT save out
    } else {
        if (searchIndex == 0) {
            tmp=new sdbf_set();
            info->index=NULL;    
        info->indexlist=NULL;
        info->setlist=NULL;
            tmp=hash_stringlist(filenames,blocksize,processing_thread_count, info);
            if (!tmp->empty()) {
        string tmp2=setname+".sdbf";
            string savename=(fs::path(home_directory)/tmp2).string();
            char *fsetname;
            fsetname=(char*)malloc(setname.length()+1);
            char *ssetname;
            ssetname=(char*)malloc(savename.length()+1);
            strncpy(ssetname,setname.c_str(),setname.length()+1);
            strncpy(fsetname,savename.c_str(),savename.length()+1);
            add_set(tmp, ssetname,hashsetID);
            std::filebuf fb;
            fb.open (fsetname,ios::out);
            if (fb.is_open()) {
                std::ostream os(&fb);
                os << tmp;
                fb.close();
                std::cout << "saveSet request succeeded "<< hashsetID << " " <<fsetname<< std::endl;
            } 
          }
      } else {
        // indexer does all the work of adding all this stuff.  statuses are
        // harder to come by.
        // generated hashsetID is ignored.
        int statusr=hash_index_stringlist(filenames,setname,blocksize,processing_thread_count);
      }
    }
     std::cout << "hashString request succeeds for "<< setname << std::endl;
  }


  void displaySourceList(std::vector<std::string> & _return) {
    std::cout << "displaySourceList" << std::endl;
    if (fs::is_directory(source_directory)){
        for ( fs::recursive_directory_iterator end, itr(source_directory); itr!= end; ++itr ) {
        //for (fs::directory_iterator itr(source_directory); itr!=fs::directory_iterator(); ++itr) {
          if (fs::is_regular_file(itr->status())) 
              _return.push_back(itr->path().string());
        }
    } else {
        _return.push_back(source_directory+" is not accessible");
    }
  }

  void getHashsetName(std::string& _return, const int32_t num1) {
    // Your implementation goes here
    if (setcollection.count(num1))
        _return=get_set_name(num1);
    else 
        _return="";
    printf("getHashsetName\n");
  }
  
  void displayResult(std::string& _return, const int32_t resultID) {
    //_return=get_request(resultID)+get_result(resultID);
    _return=get_result(resultID);
    cout << "displayResult " << resultID << endl;
  }

  void displayResultsList(std::string& _return, const std::string& user, const bool json) {
    std::stringstream resultlist;
    multimap<string,int32_t>::iterator it;
    if (json)
        resultlist << "[";
    for (it=users.equal_range(user).first; it!=users.equal_range(user).second; ++it) {
        if (json) {
            if (it!=users.equal_range(user).first)
                resultlist << ",";
            resultlist << "{\"" << (*it).second <<"\":\"";
            resultlist << get_request((*it).second) << "\"}";
        } else {
            resultlist << (*it).second << " " ;
            resultlist << get_request((*it).second) << endl;
        }
    }
    if (json)
        resultlist << "]";
    _return=resultlist.str();
    cout << "displayResultsList " << user << endl;
  }

  void displayResultStatus(std::string& _return, const int32_t resultID) {
    _return=get_result_status(resultID);
    cout << "displayResultStatus" << endl;
  }

  void displayResultDuration(std::string& _return, const int32_t resultID) {
    _return=get_result_duration(resultID);
    cout << "displayResultDuration" << endl;
  }

  int32_t createResultID(const std::string& user) {
    cout << "createResultID " << user << endl;
    return add_resultID(user);
  }

  void displayResultInfo(std::string& _return, const int32_t resultID) {
    _return=get_request(resultID);    
    cout << "displayResultInfo" << endl;
  }

  bool removeResult(const int32_t resultID) {
    cout << "removeResult" << endl;
    requests.erase(resultID);    
    results.erase(resultID);    
    starttime.erase(resultID);    
    endtime.erase(resultID);    
    multimap<string,int32_t>::iterator it;
    for (it=users.begin(); it!=users.end(); it++) {
        if  ((*it).second == resultID) {
            users.erase(it);
        }
    }
    return true; 
  }

  bool saveResult(const int32_t resultID, const std::string& result, const std::string& info) {
    // Your implementation goes here
    cout <<"saveResult" << endl;
    return false;
  }

  void shutdown() {
    printf("sdhash-srv shutdown requested by client. Shutting down.\n");
    server_->stop();
  }

  void setServer(boost::shared_ptr<TServer> server) {
    server_ = server;
  }

private:

    boost::shared_ptr<TServer> server_;

    string source_directory;
    string home_directory;
    uint32_t processing_thread_count;
    multimap<string,int32_t> users;
    map<int32_t,string> requests;
    map<int32_t,string> results;
    map<int32_t,time_t> starttime;
    map<int32_t,time_t> endtime;

    map<int32_t,sdbf_set*> setcollection;
    vector<sdbf_set*> setlist; // mirroring what is in collection for other stuff

    vector<bloom_filter *> indexlist;

    boost::random::mt19937 gen;

    void
    rng_init() {
    gen.seed(static_cast<unsigned int>(time(0)));
    }

    void
    add_result(int32_t resultID, string res) {
    results.insert(pair<int32_t,string>(resultID,res));
    endtime.insert(pair<int32_t,time_t>(resultID,time(0)));
    }

    void
    add_request(int32_t resultID, string req) {
    requests.insert(pair<int32_t,string>(resultID,req));
    starttime.insert(pair<int32_t,time_t>(resultID,time(0)));
    }

    string    
    get_request(int32_t resultID) {
    string result="";
    std::map<int32_t,string>::iterator reqfound = requests.find(resultID);
    if (reqfound!=requests.end() ) {
        string idstr=boost::lexical_cast<string>(resultID);
        result= reqfound->second;
    }
    return result;
    }

    int32_t
    add_resultID(string user) {
    boost::random::uniform_int_distribution<> dist(1,131070);
    int32_t id = dist(gen);
    users.insert(pair<string,int32_t>(user,id));
    return id;
    }

    int32_t
    make_hashsetID() {
    boost::random::uniform_int_distribution<> dist(1,131070);
    int32_t id = dist(gen);
    return id;
    }

    string
    get_result(int32_t resultID) {
        string result="";
    std::map<int32_t,string>::iterator resfound = results.find(resultID);
    std::map<int32_t,time_t>::iterator startfound = starttime.find(resultID);
    std::map<int32_t,time_t>::iterator endfound = endtime.find(resultID);
    if (resfound!=results.end() && startfound != starttime.end() && endfound != endtime.end() )  {
        time_t querytime = (endfound->second - startfound->second);
        string timestr=boost::lexical_cast<string>(querytime);
        string idstr=boost::lexical_cast<string>(resultID);
        result= resfound->second; //+"query "+idstr+" time "+timestr + " seconds\n";
    }
    return result;
    }

    string
    get_result_duration(int32_t resultID) {
    string result="";
    std::map<int32_t,time_t>::iterator startfound = starttime.find(resultID);
    std::map<int32_t,time_t>::iterator endfound = endtime.find(resultID);
    if (startfound != starttime.end() && endfound != endtime.end() )  {
        time_t querytime = (endfound->second - startfound->second);
        string timestr=boost::lexical_cast<string>(querytime);
        result= timestr + "s";
    } else if (startfound != starttime.end() && endfound == endtime.end()) {
        time_t now = time(0);
        time_t querytime = (now - startfound->second);
        string timestr=boost::lexical_cast<string>(querytime);
        result= timestr + "s";
    }    
    return result;
    }

    string    
    get_result_status(int32_t resultID) {
    string result="";
    std::map<int32_t,time_t>::iterator startfound = starttime.find(resultID);
    std::map<int32_t,time_t>::iterator endfound = endtime.find(resultID);
    if (startfound != starttime.end() && endfound != endtime.end() )  {
        std::map<int32_t,string>::iterator resultfound= results.find(resultID);
        string strresult=resultfound->second;
        uint64_t rescount=0;
        string::iterator it;
        for ( it=strresult.begin() ; it < strresult.end(); it++ )
        if (*it == '\n') rescount++;
        result="complete ("+boost::lexical_cast<string>(rescount)+")";
        
    } else if (startfound != starttime.end() && endfound == endtime.end()) {
        result="processing";
    } else {
        result="not found";
    }
    return result;
    }

    /**
    * add set and its name to the set list.
    */
    int add_set( sdbf_set *set, char *name, int32_t setID) {
    setcollection.insert(pair<int32_t,sdbf_set*>(setID,set));
    if (set->index !=NULL)
       setlist.push_back(set);
    set->set_name((std::string)name);
    set->vector_init();
    return setID;
    }

    /**
     * Returns the number of sets in our list
     */
    int get_set_count() {
    return setcollection.size();
    }

    /**
     * Returns the set associated with an id
     */
    sdbf_set *get_set( int32_t setID) {
    if (setcollection.count(setID)) {
        return setcollection.find(setID)->second;
    } else
        return NULL;
    }

    /**
     * Returns the name (string) associates with a set
     */
    std::string get_set_name( int32_t setID) {
    if( setcollection.count(setID))
        return setcollection.find(setID)->second->name();
        else
        return NULL;
    }

    /**
     * Does hashing/indexing broken up into files and saved to sets
     * with set IDs.
     */
int32_t
hash_index_stringlist(const std::vector<std::string> & filenames, string output_name, uint32_t dd_block_size,uint32_t thread_cnt) {
    std::vector<string> smallv;
    std::vector<string> large;
    for (vector<string>::const_iterator it=filenames.begin(); it < filenames.end(); it++) {
        if (fs::is_regular_file(*it)) {
            if (fs::file_size(*it) < 16*MB) {
                    smallv.push_back(*it);
            } else {
                large.push_back(*it);
            }
        }
    }
    sdbf_set *set1;
    int smallct=smallv.size();
    int largect=large.size();
    int hashfilecount=0;
    int i;
    index_info *info=(index_info*)malloc(sizeof(index_info));
    info->setlist=NULL;
    info->indexlist=NULL;
    info->search_deep=false;
    info->search_first=false;
    if (smallct > 0) {
        char **smalllist=(char **)alloc_check(ALLOC_ONLY,smallct*sizeof(char*),"main", "filename list", ERROR_EXIT);
        int filect=0;
        int sizetotal=0;
        for (i=0; i < smallct ; filect++,i++) {
            smalllist[filect]=(char*)alloc_check(ALLOC_ONLY,smallv[i].length()+1, "main", "filename", ERROR_EXIT);
            strncpy(smalllist[filect],smallv[i].c_str(),smallv[i].length()+1);
            sizetotal+=fs::file_size(smallv[i]);
            if (sizetotal > 640*MB || i==smallct-1) {
               // set up new index, set, hash them..
               bloom_filter *index1=new bloom_filter(64*MB,5,0,0.01);
               info->index=index1;
               set1=new sdbf_set(index1);
               sdbf_hash_files( smalllist, filect, thread_cnt,set1, info);
               string output_nm= (fs::path(home_directory)/output_name).string()+boost::lexical_cast<string>(hashfilecount)+".sdbf";
               sizetotal=filect=0;
               hashfilecount++;
               // print set1 && index to files with counter
               std::filebuf fb;
               fb.open (output_nm.c_str(),ios::out|ios::binary);
               if (fb.is_open()) {
                   std::ostream os(&fb);
                   os << set1;
                   fb.close();
               } else {
                   cerr << "sdhash: ERROR cannot write to file " << output_name<< endl;
                   return -1;
               }
               set1->index->set_name(output_nm);
               string output_index = output_nm + ".idx";
               int output_result = set1->index->write_out(output_index);
               if (output_result == -2) {
                   cerr << "sdhash: ERROR cannot write to file " << output_index<< endl;
                   return -1;
               }
               int32_t newid = make_hashsetID();
               indexlist.push_back(index1);
               add_set(set1,(char*)output_nm.c_str(),newid);
           }
        }
    }
    if (largect > 0) {
        int filect=0;
        int sizetotal=0;
        char **largelist=(char **)alloc_check(ALLOC_ONLY,largect*sizeof(char*),"main", "filename list", ERROR_EXIT);
        for (i=0; i < largect ; filect++, i++) {
            largelist[filect]=(char*)alloc_check(ALLOC_ONLY,large[i].length()+1, "main", "filename", ERROR_EXIT);
            strncpy(largelist[filect],large[i].c_str(),large[i].length()+1);
            sizetotal+=fs::file_size(large[i]);
            //if (sizetotal > 16*MB || i==smallct-1) {
            if (sizetotal > 640*MB || i==largect-1) {
                //if (sdbf_sys.verbose)
                //cerr << "hash "<<hashfilecount<< " numf "<<filect<< endl;
                // set up new index, set, hash them..
                //bloom_filter *index1=new bloom_filter(4*MB,5,0,0.01);
                bloom_filter *index1=new bloom_filter(64*MB,5,0,0.01);
                info->index=index1;
                set1=new sdbf_set(index1);
                // no options allowed.  this is for auto-mode.
                sdbf_hash_files_dd( largelist, filect, 16*KB, 128*MB,set1, info);
                string output_nm= output_name+boost::lexical_cast<string>(hashfilecount)+".sdbf";
                sizetotal=filect=0;
                hashfilecount++;
                // print set1 && index to files with counter
                std::filebuf fb;
                fb.open (output_nm.c_str(),ios::out|ios::binary);
                if (fb.is_open()) {
                    std::ostream os(&fb);
                    os << set1;
                    fb.close();
                } else {
                    cerr << "sdhash: ERROR cannot write to file " << output_name<< endl;
                    return -1;
                }
                set1->index->set_name(output_nm);
                string output_index = output_nm + ".idx";
                int output_result = set1->index->write_out(output_index);
                if (output_result == -2) {
                    cerr << "sdhash: ERROR cannot write to file " << output_index<< endl;
                    return -1;
                }
            int32_t newid = make_hashsetID();
            indexlist.push_back(index1);
            add_set(set1,(char*)output_nm.c_str(),newid);
            }
       }
    }

    return 0;
}
    



};

int main(int argc, char **argv) {
    sdbf_parameters_t* conf=NULL;

    conf=read_args(argc,argv,0);    

    omp_set_num_threads(conf->thread_cnt);

    if (conf==NULL) 
        return 0;
    fprintf(stderr, "sdhash-srv initializing service...\n");
    boost::shared_ptr<sdhashsrvHandler> handler(new sdhashsrvHandler(conf->thread_cnt,conf->home, conf->sources));
    boost::shared_ptr<TProcessor> processor(new sdhashsrvProcessor(handler));
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(conf->port));
    boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());


    boost::shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(conf->maxconn);
    boost::shared_ptr<PlatformThreadFactory> threadFactory = boost::shared_ptr<PlatformThreadFactory>(new PlatformThreadFactory());

    threadManager->threadFactory(threadFactory);
    threadManager->start();

    boost::shared_ptr<TServer> server = boost::shared_ptr<TServer>
    (new TThreadPoolServer(processor, serverTransport, transportFactory, protocolFactory, threadManager));
    handler->setServer(server);
    fprintf(stderr,"sdhash-srv listening on port %d, with %d worker threads.\n",conf->port, conf->maxconn);
    server->serve();
    return 0;
}

