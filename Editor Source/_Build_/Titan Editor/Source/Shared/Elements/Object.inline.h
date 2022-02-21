/******************************************************************************/
 C Memc<typename GuiEditParam>* EditObjTypeClass::findParams(C UID &type_id)
   {
      int type=find(type_id);
      return InRange(type, params) ? &params[type] : null;
   }
/******************************************************************************/
