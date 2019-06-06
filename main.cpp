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
    cout << "       fetches the resource identified by <uri> and print it" << endl;
    return -1;
  }

  /*
  Uri a("http://localhost:80/foo.html?&q=1:2:3");
  Uri b("https://localhost:80/foo.html?&q=1");
  Uri c("localhost/foo");
  Uri d("https://localhost/foo");
  Uri e("localhost:8080");
  Uri f("localhost?&foo=1");
  Uri g("localhost?&foo=1:2:3");
  Uri h("localhost?sd=:/&foo=1:2:3");
  Uri k("localhost:3000?sd=:/");
  */



  try
  {
    // prepare session
    Uri uri(argv[1]);
    Session session(uri.host(), uri.port());

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