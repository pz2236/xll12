// args.h - Arguments to register an Excel add-in
// Copyright (c) KALX, LLC. All rights reserved. No warranty made.
#pragma once
#include "defines.h"
#include "excel.h"
#include <stdlib.h>
#include <cuchar>
#include <algorithm>
#include <iterator>
#include <map>

namespace xll {

	/// See https://msdn.microsoft.com/en-us/library/office/bb687900.aspx
	enum ARG {
		ModuleText,   // from xlGetName
		Procedure,    // C function
		TypeText,     // return type and arg codes 
		FunctionText, // Excel function
		ArgumentText, // Ctrl-Shift-A text
		MacroType,    // function, 2 for macro, 0 for hidden
		Category,     // for function wizard
		ShortcutText, // single character for Ctrl-Shift-char shortcut
		HelpTopic,    // filepath!HelpContextID or http://host/path!0
		FunctionHelp, // for function wizard
		ArgumentHelp, // 1-base index
	};

	using xcstr = const XCHAR*;

	inline const OPER12& XlGetName()
	{
		static OPER12 hModule;
		
		if (hModule.xltype != xltypeStr) {
			hModule = Excel(xlGetName);
			/*
			WCHAR name[2048];
			DWORD size = 2048;
			GetModuleFileNameW(xll_hModule, name, size);
			hModule = OPER12(name);
			*/
		}

		return hModule;
	}
	/// <summary>Prepare an array suitible for <c>xlfRegister</c></summary>
	class Args {
		OPER12 args;
		mutable int arity;
	public:
		/// <summary>Number of function arguments</summary>
		int Arity() const
		{
			return arity;
		}
		/// <summary>Number of function arguments</summary>
		/// Detect the number of arguments based on the text type of a function.
		static int Arity(const OPER12& tt)
		{
			ensure (tt.type() == xltypeStr);
			XCHAR* b = tt.val.str + 1;
			XCHAR* e = b + tt.val.str[0];
			int arity = static_cast<int>(std::count_if(b, e, 
				[](const XCHAR& c) { return L'A' <= c && c <= L'U'; }
			));
			--arity; // don't count return value
			
			return arity;
		}
		/// For use as Excelv(xlfRegister, Args(....))
		operator const OPER12&() const
		{
			return args;
		}
		/// Common default.
		Args()
			: arity(0), args(1, ARG::ArgumentHelp)
		{
			std::fill(args.begin(), args.end(), OPER12(xltype::Nil));
		}
		/// Macro
		Args(xcstr Procedure, xcstr FunctionText)
			: Args()
		{
			args[ARG::ModuleText] = XlGetName();
			args[ARG::Procedure] = Procedure;
			args[ARG::FunctionText] = FunctionText;
			args[ARG::MacroType] = OPER12(2);
		}
		/// Function
		Args(xcstr TypeText, xcstr Procedure, xcstr FunctionText, int MacroType = 1)
			: Args()
		{
			args[ARG::ModuleText] = XlGetName();
			args[ARG::Procedure] = Procedure;
			args[ARG::TypeText] = TypeText;
			args[ARG::FunctionText] = FunctionText;
			args[ARG::MacroType] = MacroType;

			arity = Arity(args[ARG::TypeText]);
		}

		/// Set the name of the C/C++ function to be called.
		Args& Procedure(xcstr procedure)
		{
			args[ARG::Procedure] = procedure;

			return *this;
		}

		/// Specify the return type and argument types of the function.
		Args& TypeText(xcstr typeText)
		{
			args[ARG::TypeText] = typeText;

			return *this;
		}
		/// Specify the name of the function or macro to be used by Excel.
		Args& FunctionText(xcstr functionText)
		{
			args[ARG::FunctionText] = functionText;

			return *this;
		}
		const OPER12& FunctionText() const
		{
			return args[ARG::FunctionText];
		}
		/// Specify the macro type of the function.
		/// Use 1 for functions, 2 for macros, and 0 for hidden functions. 
		Args& MacroType(int macroType)
		{
			args[ARG::MacroType] = macroType;

			return *this;
		}
		/// Hide the name of the function from Excel.
		Args& Hidden()
		{
			return MacroType(0);
		}
		/// Set the category to be used in the function wizard.
		Args& Category(xcstr category)
		{
			args[ARG::Category] = category;

			return *this;
		}
		/// Specify the shortcut text for calling the function.
		Args& ShortcutText(XCHAR shortcutText)
		{
			args[ARG::ShortcutText] = OPER12(&shortcutText, 1);

			return *this;
		}
		/// Specify the help topic to be used in the Function Wizard.
		/// !!!This must have the format...
		Args& HelpTopic(xcstr helpTopic)
		{
			args[ARG::HelpTopic] = helpTopic;

			return *this;
		}
		/// Specify the function help displayed in the Functinon Wizard.
		Args& FunctionHelp(xcstr functionHelp)
		{
			args[ARG::FunctionHelp] = functionHelp;

			return *this;
		}
		/// Specify individual argument help in the Function Wizard.
		Args& ArgumentHelp(int i, xcstr argumentHelp)
		{
			ensure (i != 0);

			if (args.size() < ARG::ArgumentHelp + i)
				args.resize(i, ARG::ArgumentHelp + i);

			args[ARG::ArgumentHelp + i - 1] = argumentHelp;

			return *this;
		}
		/// Add an individual argument.
		Args& Arg(xcstr type, xcstr text, xcstr helpText = nullptr)
		{
			OPER12& Type = args[ARG::TypeText];
			Type &= type;
			
			OPER12& Text = args[ARG::ArgumentText];
			if (arity > 0)
				Text &= L", ";
			Text &= text;
			
			++arity;
			if (helpText && *helpText)
				ArgumentHelp(arity, helpText);

			return *this;
		}
		/// Argument modifiers
		Args& Threadsafe()
		{
			args[ARG::TypeText] &= XLL_THREAD_SAFE;

			return *this;
		}
		Args& Uncalced()
		{
			args[ARG::TypeText] &= XLL_UNCALCED;

			return *this;
		}
		Args& Volatile()
		{
			args[ARG::TypeText] &= XLL_VOLATILE;

			return *this;
		}
		/// Convenience function for number types.
		Args& Num(xcstr text, xcstr helpText = nullptr)
		{
			return Arg(XLL_DOUBLE, text, helpText);
		}
		// Str ...
	};

	using Function = Args;
	using Macro = Args;

	/// Array appropriate for xlfRegister.
	/// Use like <c>Excelv(xlfRegister, Arguments(...))</c>
	inline OPER12 Arguments(
		xcstr Procedure,        // C function
		xcstr TypeText,         // return type and arg codes 
		xcstr FunctionText,     // Excel function
		xcstr ArgumentText = 0, // Ctrl-Shift-A text
		int   MacroType = 1,    // function, 2 for macro, 0 for hidden
		xcstr Category = 0,     // for function wizard
		xcstr ShortcutText = 0, // single character for Ctrl-Shift-char shortcut
		xcstr HelpTopic = 0,    // filepath!HelpContextID or http://host/path!0
		xcstr FunctionHelp = 0, // for function wizard
		xcstr ArgumentHelp1 = 0,
		xcstr ArgumentHelp2 = 0,
		xcstr ArgumentHelp3 = 0,
		xcstr ArgumentHelp4 = 0,
		xcstr ArgumentHelp5 = 0,
		xcstr ArgumentHelp6 = 0,
		xcstr ArgumentHelp7 = 0,
		xcstr ArgumentHelp8 = 0,
		xcstr ArgumentHelp9 = 0
	)
	{
		OPER12 args(Excel(xlGetName));
		args.push_back(OPER12(Procedure));
		args.push_back(OPER12(TypeText));
		args.push_back(OPER12(FunctionText));
		args.push_back(OPER12(ArgumentText));
		args.push_back(OPER12(MacroType));
		args.push_back(OPER12(Category));
		args.push_back(OPER12(ShortcutText));
		args.push_back(OPER12(HelpTopic));
		args.push_back(OPER12(FunctionHelp));
		args.push_back(OPER12(ArgumentHelp1));
		args.push_back(OPER12(ArgumentHelp2));
		args.push_back(OPER12(ArgumentHelp3));
		args.push_back(OPER12(ArgumentHelp4));
		args.push_back(OPER12(ArgumentHelp5));
		args.push_back(OPER12(ArgumentHelp6));
		args.push_back(OPER12(ArgumentHelp7));
		args.push_back(OPER12(ArgumentHelp8));
		args.push_back(OPER12(ArgumentHelp9));

		return args;
	}
	/*
	// Convert __FUNCDNAME__ to arguments for xlfRegister
	inline Args Demangle(const XCHAR* F)
	{
		static std::map<XCHAR,const OPER12> arg_map = {
			{ L'F', OPER12(XLL_SHORT) },
			{ L'G', OPER12(XLL_WORD) }, // also USHORT
			{ L'H', OPER12(XLL_BOOL) },
			{ L'J', OPER12(XLL_LONG) },
			{ L'N', OPER12(XLL_DOUBLE) },
		};

		// C to Excel naming convention
		auto function_text = [](OPER12 o) {
			ensure (o.type() == xltypeStr);
			for (int i = 1; i <= o.val.str[0]; ++i) {
				if (o.val.str[i] == L'_')
					o.val.str[i] = L'.';
				else
					o.val.str[i] = ::towupper(o.val.str[i]);
			}

			return o;
		};

		Args args;
		args.MacroType(1); // function
								  
		// "?foo@@YGNN@Z"

		ensure (F && *F == '?');
		auto E = wcschr(F, L'@');
		ensure (E);
		args.Procedure(OPER12(F, E - F));
		args.FunctionText(function_text(OPER12(F + 1, E - F - 1)));

		F = E;
		ensure (*++F == L'@');
		ensure (*++F == L'Y');
		ensure (*++F == L'G');
		ensure (arg_map.find(*++F) != arg_map.end());
		args.TypeText(arg_map[*F]);
		while (*++F != L'@') {
			// if (*F == L'P') { pointer...
			ensure (arg_map.find(*F) != arg_map.end());
//			type = Excel(xlfConcatenate, type, OPER12(arg_map[*F]));
		}

		return args;
	}
	*/
} // xll