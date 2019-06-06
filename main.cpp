#include "uri.h"
#include "session.h"

#include <iostream>
#include <exception>

using namespace std;

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    cout << "Usage: " << argv[0] << " <uri>" << endl;
    cout << "       fetches the file identified by <uri> and save it" << endl;
    return -1;
  }

  try
  {
    // prepare session
    Uri uri(argv[1]);
    if(uri.scheme() != "http"){
      cout << uri.scheme() << " not supported" << endl;
      return -1;
    }
    Session session;
    session.init(uri.host(), uri.port());

    // prepare path
    string path(uri.path());
    if (path.empty()) path = "/";

    // send request
    session.sendRequest(uri.host(), uri.port(), path);

    session.readAndSave("file.out");
  }
  catch (exception &ex)
  {
    cerr << ex.what() << endl;
    return -1;
  }

  return 0;
}