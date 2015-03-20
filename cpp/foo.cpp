// Not supported by apple clang?
#define GC_ATTR_ALLOC_SIZE(x)

#include <gc/gc_cpp.h>
#include <string>

struct MalType;
struct MalList;
struct MalSeq;
struct MalString;
class Env;

void init_mal();

// Create an error message for throwing.
MalString* error(std::string s);

template <typename T> inline std::string print_type();

bool equal(MalType* a, MalType* b);

struct MalType : public gc {
  virtual ~MalType() { }
  virtual std::string print(bool print_readably = true) const = 0;
private:
  // Implementations of equal_impl may safely static_cast to their own type.
  // They can also assume that the argument is different from `this`.
  virtual bool equal_impl(MalType*) const = 0;
  friend bool ::equal(MalType*, MalType*);
};
template<> inline std::string print_type<MalType>() { return "Object"; }

struct MalTrue : public MalType {
  MalTrue() { }
  bool equal_impl(MalType*) const { return true; }
  std::string print(bool = true) const { return "true"; }
};
extern MalTrue* _true;

MalType* symbol() {
	static MalType* p = nullptr;
	if (!p) p = new MalTrue;
    return p;
}

class Foo : public gc { };

int main(int argc, char* argv[]) {
  auto f = new Foo();
  auto t = new MalTrue();
  static MalTrue* pt = new MalTrue();
}