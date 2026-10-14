// Byte-exact VUC Value::operator|| (309/309 original instructions).
// The four vector component-pair OR results are ANDed together.
// Reconstructed Value ABI derives from independently exact relational units.
// The precise historical original compiler flags/source unit remain unproven.
struct Value {
 int type; unsigned a,b; unsigned char flag0,flag1,flag2,flag3;
 union { int bits; float values[4]; };
 Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}
 static bool TypeConvert(const Value&,const Value&,Value&,Value&);
 Value operator&&(const Value&) const;
 Value operator||(const Value&) const;
};

Value Value::operator||(const Value &other) const {
 Value result;
 Value temporary;
 result.type=2;
 if (TypeConvert(*this,other,result,temporary)) {
  switch (result.type) {
   case 1: case 2: result.bits=(result.bits || temporary.bits);break;
   case 3: result.bits=(result.values[0] != 0.0f || temporary.values[0] != 0.0f);break;
   case 4: result.bits=((result.values[0] != 0.0f || temporary.values[0] != 0.0f)) && ((result.values[1] != 0.0f || temporary.values[1] != 0.0f)) && ((result.values[2] != 0.0f || temporary.values[2] != 0.0f)) && ((result.values[3] != 0.0f || temporary.values[3] != 0.0f));break;
  }
 }
 return result;
}
