NO_INLINE JsVar *jspeFactorFunctionCall() {
  /* The parent if we're executing a method call */
  bool isConstructor = false;
  if (lex->tk==LEX_R_NEW) {
    JSP_ASSERT_MATCH(LEX_R_NEW);
    isConstructor = true;

    if (lex->tk==LEX_R_NEW) {
      jsExceptionHere(JSET_ERROR, "Nesting 'new' operators is unsupported");
      jspSetError(false);
      return 0;
    }
  }

  JsVar *parent = 0;
  JsVar *a = 0;

#ifndef ESPR_NO_CLASSES
  bool hasSetCurrentClassConstructor = false;
  if (lex->tk==LEX_R_SUPER) {
    JSP_ASSERT_MATCH(LEX_R_SUPER);
    /* We would normally handle this in jspeFactor, but we're doing it here
     as we have access to parent, which we need to be able to set because
     super() is a bit like calling 'this.super()'. We also need to do some
     hacky stuff to ensure

     This is kind of nasty, since super appears to do
     three different things depending on what 'this' is.

      * In the constructor it references the extended class's constructor
      * in a method it references the constructor's prototype.
      * in a static method it references the extended class's constructor (but this is different)
     */
    if (jsvIsObject(execInfo.thisVar)) {
      // 'this' is an object - must be calling a normal method
      JsVar *proto1;
      if (execInfo.currentClassConstructor && lex->tk=='(') {
        // If we're doing super() we want the constructor. We have to be careful
        // we don't keep asking for the same one so don't get it from 'this' which has to be the
        // same each time https://github.com/espruino/Espruino/issues/1529
        proto1 = jsvObjectGetChildIfExists(execInfo.currentClassConstructor, JSPARSE_PROTOTYPE_VAR);
      } else {
        proto1 = jsvObjectGetChildIfExists(execInfo.thisVar, JSPARSE_INHERITS_VAR); // if we're in a method, get __proto__ first

      }
      JsVar *proto2 = jsvIsObject(proto1) ? jsvObjectGetChildIfExists(proto1, JSPARSE_INHERITS_VAR) : 0; // still in method, get __proto__.__proto__
      jsvUnLock(proto1);
      if (!proto2) {
        jsExceptionHere(JSET_SYNTAXERROR, "Calling 'super' outside of class");
        return 0;
      }
      // If we're doing super() we want the constructor
      if (lex->tk=='(') {
        JsVar *constr = jsvObjectGetChildIfExists(proto2, JSPARSE_CONSTRUCTOR_VAR);
        jsvUnLock(proto2);
        execInfo.currentClassConstructor = constr;
        hasSetCurrentClassConstructor = true;
        a = constr;
      } else {
        // But if we're doing something else - eg 'super.' or 'super[' then it needs to reference the prototype
        a = proto2;
      }
    } else if (jsvIsFunction(execInfo.thisVar)) {
      // 'this' is a function - must be calling a static method
      JsVar *proto1 = jsvObjectGetChildIfExists(execInfo.thisVar, JSPARSE_PROTOTYPE_VAR);
      JsVar *proto2 = jsvIsObject(proto1) ? jsvObjectGetChildIfExists(proto1, JSPARSE_INHERITS_VAR) : 0;
      jsvUnLock(proto1);
      if (!proto2) {
        jsExceptionHere(JSET_SYNTAXERROR, "Calling 'super' outside of class");
        return 0;
      }
      JsVar *constr = jsvObjectGetChildIfExists(proto2, JSPARSE_CONSTRUCTOR_VAR);
      jsvUnLock(proto2);
      a = constr;
    } else {
      jsExceptionHere(JSET_SYNTAXERROR, "Calling 'super' outside of class");
      return 0;
    }
    // search for member accesses eg 'super.xyz'...
    JsVar *superVar = a;
    a = jspeFactorMember(a, &parent);
    // Set the parent to 'this' if it was undefined or equal to the 'super' var
    if (!parent || parent==superVar) {
      jsvUnLock(parent);
      parent = jsvLockAgain(execInfo.thisVar);
    }
  } else
#endif
  a = jspeFactorMember(jspeFactor(), &parent);

  while ((lex->tk=='(' || (isConstructor && JSP_SHOULD_EXECUTE)) && !jspIsInterrupted()) {
    JsVar *funcName = a;
    JsVar *func = jsvSkipName(funcName);
    if (!func)  { // could have ReferenceErrored while skipping name
      jsvUnLock2(funcName, parent);
      return 0;
    }
    /* The constructor function doesn't change parsing, so if we're
    * not executing, just short-cut it. */
    if (isConstructor && JSP_SHOULD_EXECUTE) {
      // If we have '(' parse an argument list, otherwise don't look for any args
      bool parseArgs = lex->tk=='(';
      a = jspeConstruct(func, funcName, parseArgs);
      isConstructor = false; // don't treat subsequent brackets as constructors
    } else
      a = jspeFunctionCall(func, funcName, parent, true, 0, 0);
    jsvUnLock3(funcName, func, parent);
    parent=0;
    a = jspeFactorMember(a, &parent);
  }
#ifndef ESPR_NO_CLASSES
  if (hasSetCurrentClassConstructor)
    execInfo.currentClassConstructor = 0;
#endif

#ifndef ESPR_NO_GET_SET
  /* If we've got something that we care about the parent of (eg. a getter/setter)
   * then we repackage it into a 'NewChild' name that references the parent before
   * we leave. Note: You can't do this on everything because normally NewChild
   * forces a new child to be blindly created. It works on Getters/Setters because
   * we *always* run those rather than adding them.
   */
  if (parent && jsvIsBasicName(a) && !jsvIsNewChild(a)) {
    JsVar *value = jsvLockSafe(jsvGetFirstChild(a));
    if (jsvIsGetterOrSetter(value)) { // no need to do this for functions since we've just executed whatever we needed to
      JsVar *nameVar = jsvCopyNameOnly(a,false,true);
      JsVar *newChild = jsvCreateNewChild(parent, nameVar, value);
      jsvUnLock2(nameVar, a);
      a = newChild;
    }
    jsvUnLock(value);
  }
#endif
  jsvUnLock(parent);
  return a;
}