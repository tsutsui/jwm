/**
 * @file mouse.c
 * @author Joe Wingbermuehle
 * @date 2013
 *
 * @brief Mouse binding functions.
 *
 */

#include "jwm.h"
#include "mouse.h"
#include "key.h"
#include "misc.h"
#include "binding.h"
#include "client.h"
#include "settings.h"
#include "main.h"

typedef struct MouseNode {
   int button;
   ContextType context;
   ActionNode action;
   unsigned int state;
   struct MouseNode *next;
} MouseNode;

static MouseNode *bindings[CONTEXT_COUNT];
static ReleaseCallback callback;
static void *callbackArg;

/** Initialize mouse bindings. */
void InitializeMouse()
{
   int i;
   for(i = 0; i < CONTEXT_COUNT; i++) {
      bindings[i] = NULL;
   }
   callback = NULL;
}

/** Clean up mouse bindings. */
void DestroyMouse()
{
   MouseNode *mp;
   int i;
   for(i = 0; i < CONTEXT_COUNT; i++) {
      while(bindings[i]) {
         mp = bindings[i]->next;
         if(bindings[i]->action.arg) {
            Release(bindings[i]->action.arg);
         }
         Release(bindings[i]);
         bindings[i] = mp;
      }
   }
}

/** Check for and process a mouse grab. */
char CheckMouseGrab(const XButtonEvent *event)
{
   if(event->type == ButtonRelease && callback) {
      JXUngrabPointer(display, CurrentTime);
      (callback)(event, callbackArg);
      callback = NULL;
      return 1;
   } else {
      return 0;
   }
}

/** Run mouse bindings for an event. */
char RunMouseCommand(const XButtonEvent *event,
                     ContextType context,
                     const ActionContext *ac)
{

   static int lastX = 0, lastY = 0;
   static Time lastClickTime;
   static char doubleClickActive = 0;

   const unsigned int state = event->state & lockMask;
   MouseNode *mp;
   int button;
   char menuShown;

   button = event->button;
   if(event->type == ButtonPress) {
      if(doubleClickActive &&
         abs(event->time - lastClickTime) > 0 &&
         abs(event->time - lastClickTime) <= settings.doubleClickSpeed &&
         abs(event->x_root - lastX) <= settings.doubleClickDelta &&
         abs(event->y_root - lastY) <= settings.doubleClickDelta) {
         doubleClickActive = 0;
         button *= 11;
      } else {
         doubleClickActive = 1;
         lastX = event->x_root;
         lastY = event->y_root;
         lastClickTime = event->time;
      }
   }

   menuShown = 0;
   mp = bindings[context];
   while(mp) {
      if(button == mp->button && state == mp->state) {
         if(RunAction(ac, &mp->action)) {
            menuShown = 1;
         }
      }
      mp = mp->next;
   }

   return menuShown;

}

/** Set the callback for a mouse grab. */
void SetButtonReleaseCallback(ReleaseCallback c, void *a)
{
   callback = c;
   callbackArg = a;
}

/** Insert a mouse binding. */
void InsertMouseBinding(ContextType context,
                        int button,
                        const char *modifiers,
                        const ActionNode *action)
{

   MouseNode *mp;

   mp = Allocate(sizeof(MouseNode));
   mp->next = bindings[context];
   bindings[context] = mp;

   mp->context          = context;
   mp->button           = button;
   mp->action           = *action;
   mp->state            = ParseModifierString(modifiers);

   switch(mp->action.type) {
   case ACTION_ROOT:
   case ACTION_WIN:
   case ACTION_MOVE:
   case ACTION_RESIZE:
      break;
   default:
      switch(mp->button) {
      case Button1:     mp->state |= Button1Mask;  break;
      case Button2:     mp->state |= Button2Mask;  break;
      case Button3:     mp->state |= Button3Mask;  break;
      case Button4:     break;
      case Button5:     break;
      default:          break;
      }
      break;
   }

}
