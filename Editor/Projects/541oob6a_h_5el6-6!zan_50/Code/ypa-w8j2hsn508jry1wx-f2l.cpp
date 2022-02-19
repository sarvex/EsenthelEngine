/******************************************************************************

   !! BEFORE RUNNING THIS TUTORIAL, PLEASE READ THE INSTRUCTIONS ON HOW TO SETUP XBOX LIVE !!
   !! INSTRUCTIONS ARE IN "Engine\Net\Xbox Live" Header File                      !!

/******************************************************************************/
const bool IHaveReadInstructionsAtTheTop=false; // read the instructions above, then set this to true

InternetCache IC; // cache for images from the internet
Threads       Workers; // worker threads for importing images in the background thread
/******************************************************************************/
void XboxCallback(XBOXLive.EVENT event, ULong user_id)
{
   switch(event)
   {
      case XBOXLive.STATUS_CHANGED:
      {
         if(user_id==XboxLive.userID() && XboxLive.loggedIn()) // after successful log in
         {
            XboxLive.getFriends     (); // request list of friends
            XboxLive.getAchievements(); // request list of achievements
         }
      }break;
   }
}
/******************************************************************************/
void InitPre()
{
   ASSERT(IHaveReadInstructionsAtTheTop);
   INIT();
   XboxLive.callback=XboxCallback;
   XboxLive.logIn();
}
/******************************************************************************/
bool Init()
{
   Workers.create(true, 1); // create workers
   IC.create(S, &Workers); // create internet cache
   Images.delayRemove(30); // this will delay automatic unloading of cached images
   return true;
}
void Shut()
{
}
/******************************************************************************/
cchar8* TextOK(bool ok) {return ok ? "SUCCESS" : "FAIL";}
void TestCloud()
{
   Str msg;

   // save
   {
      Str text="Text";
      File f; f.writeMem().putStr(text).pos(0);
      msg.line()+=S+"Save \""+text+"\": "+TextOK(XboxLive.cloudSave("save", f));
   }
   // load
   {
      File f;
      bool ok=XboxLive.cloudLoad("save", f, true); f.pos(0);
      Str  text=f.getStr();
      msg.line()+=S+"Load: "+TextOK(ok)+", text: \""+text+"\"";
   }
   // list files
   {
      Memt<XBOXLive.CloudFile> files;
      XboxLive.cloudFiles(files);
      msg.line()+=S+"Cloud Files: "+files.elms();
   }
   // delete
   {
      msg.line()+=S+"Delete File: "+TextOK(XboxLive.cloudDel("save"));
   }
   // list files
   {
      Memt<XBOXLive.CloudFile> files;
      XboxLive.cloudFiles(files);
      msg.line()+=S+"Cloud Files: "+files.elms();
   }

   Gui.msgBox("XboxLive Cloud Test Results", msg);
}
/******************************************************************************/
bool Update()
{
   Gui.update();
   if(Kb.bp(KB_ESC))return false;
   if(Kb.bp(KB_ENTER))TestCloud();
   return true;
}
/******************************************************************************/
void Draw()
{
   D.clear(AZURE);
   
   Str status;
   switch(XboxLive.status())
   {
      case XBOXLive.LOGGED_OUT: status="Logged Out"; break;
      case XBOXLive.LOGGING_IN: status="Logging In"; break;
      case XBOXLive.LOGGED_IN : status="Logged In" ; break;
   }
   flt h=0.07, y=D.h()-h;
   D.text(0, y, status); y-=h;

   if(XboxLive.status()==XBOXLive.LOGGED_IN)
   {
      D.text(0, y, S+"Welcome "+XboxLive.userName()); y-=h;
      if(ImagePtr photo=IC.getImage(XboxLive.userImageURL()))photo->drawFit(Rect_U(0, y, 0.3, 0.3)); y-=h+0.3;

      D.text(0, y, S+"Cloud supported:"+XboxLive.cloudSupported()+", available size:"+XboxLive.cloudAvailableSize()); y-=h;
      D.text(0, y, "Press Enter to test Cloud Saves"); y-=h;
      y-=h;

      flt x=-D.w()/2;
      D.text(0, y, S+"Friends:"); y-=h;
      Memt<XBOXLive.Friend> friends; if(XboxLive.getFriends(friends))
      {
         flt h=0.06; TextStyleParams ts; ts.align.x=1; ts.size=h;
         FREPA(friends)
         {
          C XBOXLive.Friend &user=friends[i];
            D.text(ts, x, y, S+'#'+i+", ID:"+user.id+", Name:"+user.name+(user.favorite ? "(*)" : null));
            if(ImagePtr photo=IC.getImage(user.image_url))photo->drawFit(Rect_R(x-0.05, y, h, h));
            y-=h;
         }
      }else 
      {
         D.text(0, y, "Unknown"); y-=h;
      }
      y-=h/2;

      D.text(0, y, S+"Achievements:"); y-=h;
      Memt<XBOXLive.Achievement> achievements; if(XboxLive.getAchievements(achievements))
      {
         flt h=0.06; TextStyleParams ts; ts.align.x=1; ts.size=h;
         FREPA(achievements)
         {
          C XBOXLive.Achievement &achievement=achievements[i];
            D.text(ts, x, y, S+'#'+i+", ID:"+achievement.id+", Name:\""+achievement.name+"\", Progress:"+achievement.progress+", Unlocked:"+achievement.unlocked());
            y-=h;
         }
      }else 
      {
         D.text(0, y, "Unknown"); y-=h;
      }
      y-=h/2;
   }
   Gui.draw();
}
/******************************************************************************/
