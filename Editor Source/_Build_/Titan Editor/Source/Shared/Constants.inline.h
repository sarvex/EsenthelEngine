/******************************************************************************/
   bool TimeStamp::is()C {return u!=0;}
   uint TimeStamp::text()C {return u;}
   TimeStamp& TimeStamp::zero() {u=0;                   return T;}
   bool TimeStamp::operator==(C TimeStamp &t)C {return u==t.u;}
   bool TimeStamp::operator!=(C TimeStamp &t)C {return u!=t.u;}
   bool TimeStamp::operator>=(C TimeStamp &t)C {return u>=t.u;}
   bool TimeStamp::operator<=(C TimeStamp &t)C {return u<=t.u;}
   bool TimeStamp::operator> (C TimeStamp &t)C {return u> t.u;}
   bool TimeStamp::operator< (C TimeStamp &t)C {return u< t.u;}
/******************************************************************************/
