// EXPERIMENTAL VUC Value::operator&&, 190/325 original instruction bytes.
// Original 325-byte size and TypeConvert relocation verified with both GCC
// 3.3.3 prereleases. Not accepted as exact source; original compiler layout
// and vector basic-block ordering still differ from this reconstruction.
struct Value {
 int type; unsigned a,b; unsigned char flag0,flag1,flag2,flag3;
 union { int bits; float values[4]; };
 Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}
 static bool TypeConvert(const Value&,const Value&,Value&,Value&);
 Value operator&&(const Value&) const;
 Value operator||(const Value&) const;
};

Value Value::operator&&(const Value &other) const {
 Value result;
 Value temporary;
 result.type=2;
 if (TypeConvert(*this,other,result,temporary)) {
  switch (result.type) {
   case 1: case 2: result.bits=(result.bits && temporary.bits);break;
   case 3: result.bits=(result.values[0] != 0.0f && temporary.values[0] != 0.0f);break;
   case 4: result.bits=((result.values[0] != 0.0f && temporary.values[0] != 0.0f)) && ((result.values[1] != 0.0f && temporary.values[1] != 0.0f)) && ((result.values[2] != 0.0f && temporary.values[2] != 0.0f)) && ((result.values[3] != 0.0f && temporary.values[3] != 0.0f));break;
  }
 }
 return result;
}
