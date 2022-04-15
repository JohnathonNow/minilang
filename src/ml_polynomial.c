#include "ml_polynomial.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include <math.h>
#include <float.h>
#include "ml_sequence.h"

//#define ML_POLY_DEBUG

static stringmap_t Variables[1] = {STRINGMAP_INIT};
static const char **Names = 0;

const char *ml_polynomial_name(int Index) {
	if (Index < 0 || Index >= Variables->Size) return NULL;
	return Names[Index];
}

static ml_factors_t Constant[1] = {{0, 0}};

static int ml_factors_cmp(const ml_factors_t *A, const ml_factors_t *B) {
	if (A->Degree < B->Degree) return -1;
	if (A->Degree > B->Degree) return 1;
	const ml_factor_t *FA = A->Factors, *FB = B->Factors;
	if (A->Count < B->Count) {
		for (int I = 0; I < A->Count; ++I, ++FA, ++FB) {
			if (FA->Variable > FB->Variable) return -1;
			if (FA->Variable < FB->Variable) return 1;
			if (FA->Degree < FB->Degree) return -1;
			if (FA->Degree > FB->Degree) return 1;
		}
		return -1;
	} else if (A->Count > B->Count) {
		for (int I = 0; I < B->Count; ++I, ++FA, ++FB) {
			if (FA->Variable > FB->Variable) return -1;
			if (FA->Variable < FB->Variable) return 1;
			if (FA->Degree < FB->Degree) return -1;
			if (FA->Degree > FB->Degree) return 1;
		}
		return 1;
	} else {
		for (int I = 0; I < A->Count; ++I, ++FA, ++FB) {
			if (FA->Variable > FB->Variable) return -1;
			if (FA->Variable < FB->Variable) return 1;
			if (FA->Degree < FB->Degree) return -1;
			if (FA->Degree > FB->Degree) return 1;
		}
		return 0;
	}
}

static const ml_factors_t *ml_factors_mul(const ml_factors_t *A, const ml_factors_t *B) {
	if (A->Count == 0) return B;
	if (B->Count == 0) return A;
	ml_factors_t *C = xnew(ml_factors_t, A->Count + B->Count, ml_factor_t);
	const ml_factor_t *FA = A->Factors, *FB = B->Factors;
	ml_factor_t *FC = C->Factors;
	int CA = A->Count, CB = B->Count;
	int Degree = 0;
	while (CA && CB) {
		if (FA->Variable < FB->Variable) {
			Degree += FA->Degree;
			*FC++ = *FA++;
			--CA;
		} else if (FA->Variable > FB->Variable) {
			Degree += FB->Degree;
			*FC++ = *FB++;
			--CB;
		} else {
			if ((FC->Degree = FA->Degree + FB->Degree)) {
				Degree += FC->Degree;
				FC->Variable = FA->Variable;
				++FC;
			}
			++FA; ++FB;
			--CA; --CB;
		}
	}
	while (CA) {
		Degree += FA->Degree;
		*FC++ = *FA++;
		--CA;
	}
	while (CB) {
		Degree += FB->Degree;
		*FC++ = *FB++;
		--CB;
	}
	C->Count = FC - C->Factors;
	C->Degree = Degree;
	return C;
}

static ml_value_t *ml_polynomial_value(ml_polynomial_t *P) {
	if (P->Count == 0) return ml_real(0);
	if (P->Count == 1 && P->Terms->Factors->Count == 0) return ml_real(P->Terms->Coeff);
	return (ml_value_t *)P;
}

void ml_polynomial_write(ml_stringbuffer_t *Buffer, ml_polynomial_t *Poly) {
const ml_term_t *Terms = Poly->Terms;
	for (int I = 0; I < Poly->Count; ++I) {
		const ml_term_t *Term = Terms + I;
		if (Term->Factors->Count == 0) {
			if (I && Term->Coeff < 0) {
				ml_stringbuffer_printf(Buffer, " - %g", -Term->Coeff);
			} else if (I) {
				ml_stringbuffer_printf(Buffer, " + %g", Term->Coeff);
			} else {
				ml_stringbuffer_printf(Buffer, "%g", Term->Coeff);
			}
		} else {
			if (fabs(Term->Coeff - 1) < DBL_EPSILON) {
				if (I) {
					ml_stringbuffer_write(Buffer, " + ", 3);
				}
			} else if (fabs(Term->Coeff + 1) < DBL_EPSILON) {
				if (I) {
					ml_stringbuffer_write(Buffer, " - ", 3);
				} else {
					ml_stringbuffer_write(Buffer, "-", 1);
				}
			} else {
				if (I && Term->Coeff < 0) {
					ml_stringbuffer_printf(Buffer, " - %g", -Term->Coeff);
				} else if (I) {
					ml_stringbuffer_printf(Buffer, " + %g", Term->Coeff);
				} else {
					ml_stringbuffer_printf(Buffer, "%g", Term->Coeff);
				}
			}
			const ml_factor_t *Factor = Term->Factors->Factors;
			for (int J = Term->Factors->Count; --J >= 0; ++Factor) {
				ml_stringbuffer_printf(Buffer, "%s", Names[Factor->Variable - 1]);
				if (Factor->Degree != 1) {
					char Degree[16];
					sprintf(Degree, "%d", Factor->Degree);
					static const char *Exponents[10] = {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};
					for (char *D = Degree; *D; ++D) ml_stringbuffer_printf(Buffer, "%s", Exponents[*D - '0']);
				}
			}
		}
	}
}

typedef struct ml_substitution_t ml_substitution_t;

struct ml_substitution_t {
	int Variable, Degree;
	ml_value_t *Values[];
};

typedef struct {
	ml_state_t Base;
	ml_value_t *Args[3];
	ml_polynomial_t *P;
	ml_factors_t *F;
	inthash_t Subs[1];
	int I1, I2;
} ml_polynomial_call_state_t;

extern ml_value_t *MulMethod;
extern ml_value_t *AddMethod;

static void ml_polynomial_factor_run(ml_polynomial_call_state_t *State, ml_value_t *Value);
static void ml_polynomial_term_run(ml_polynomial_call_state_t *State, ml_value_t *Value);
static void ml_polynomial_call_term(ml_polynomial_call_state_t *State);

static void ml_polynomial_call_factor(ml_polynomial_call_state_t *State) {
	ml_polynomial_t *P = State->P;
	ml_term_t Term = P->Terms[State->I1];
	const ml_factors_t *F = Term.Factors;
	if (State->I2 > F->Count) {
		++State->I1;
		if (State->Args[0]) {
			State->Base.run = (ml_state_fn)ml_polynomial_term_run;
			return ml_call(State, AddMethod, 2, State->Args);
		} else {
			return ml_polynomial_term_run(State, State->Args[1]);
		}
	}
	ml_factors_t *F2 = State->F;
	for (int I = State->I2; I < F->Count; ++I) {
		ml_substitution_t *Sub = inthash_search(State->Subs, F->Factors[I].Variable);
		if (Sub) {
			State->I2 = I + 1;
			State->Args[2] = Sub->Values[F->Factors[I].Degree - 1];
			State->Base.run = (ml_state_fn)ml_polynomial_factor_run;
			return ml_call(State, MulMethod, 2, State->Args + 1);
		} else if (F2) {
			F2->Factors[F2->Count++] = F->Factors[I];
			F2->Degree += F->Factors[I].Degree;
		} else {
			F2 = State->F = xnew(ml_factors_t, F->Count - I, ml_factor_t);
			F2->Factors[0] = F->Factors[I];
			F2->Degree += F->Factors[I].Degree;
			F2->Count = 1;
		}
	}
	State->I2 = F->Count + 1;
	if (F2) {
		ml_polynomial_t *Q = xnew(ml_polynomial_t, 1, ml_term_t);
		Q->Type = MLPolynomialT;
		Q->Count = 1;
		Q->Terms->Coeff = Term.Coeff;
		Q->Terms->Factors = F2;
		State->Args[2] = (ml_value_t *)Q;
		State->Base.run = (ml_state_fn)ml_polynomial_factor_run;
		return ml_call(State, MulMethod, 2, State->Args + 1);
	} else {
		State->Args[2] = ml_real(Term.Coeff);
		State->Base.run = (ml_state_fn)ml_polynomial_factor_run;
		return ml_call(State, MulMethod, 2, State->Args + 1);
	}
}

static void ml_polynomial_factor_run(ml_polynomial_call_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[1] = Value;
	return ml_polynomial_call_factor(State);
}

static void ml_polynomial_call_term(ml_polynomial_call_state_t *State) {
	ml_polynomial_t *P = State->P;
	if (State->I1 == P->Count) ML_CONTINUE(State->Base.Caller, State->Args[0]);
	ml_term_t Term = P->Terms[State->I1];
	if (!Term.Factors->Count) {
		if (State->Args[0]) {
			State->Args[1] = ml_real(Term.Coeff);
			return ml_call(State->Base.Caller, AddMethod, 2, State->Args);
		} else {
			ML_CONTINUE(State->Base.Caller, ml_real(Term.Coeff));
		}
	}
	State->F = NULL;
	const ml_factors_t *F = Term.Factors;
	for (int I = 0; I < F->Count; ++I) {
		ml_substitution_t *Sub = inthash_search(State->Subs, F->Factors[I].Variable);
		if (Sub) {
			if (I) {
				ml_factors_t *F2 = State->F = xnew(ml_factors_t, F->Count - 1, ml_factor_t);
				int Degree = 0;
				for (int J = 0; J < I; ++J) {
					F2->Factors[J] = F->Factors[J];
					Degree += F->Factors[J].Degree;
				}
				F2->Degree = Degree;
				F2->Count = I;
			}
			State->Args[1] = Sub->Values[F->Factors[I].Degree - 1];
			State->I2 = I + 1;
			return ml_polynomial_call_factor(State);
		}
	}
	++State->I1;
	ml_polynomial_t *Q = xnew(ml_polynomial_t, 1, ml_term_t);
	Q->Type = MLPolynomialT;
	Q->Count = 1;
	Q->Terms[0] = Term;
	if (State->Args[0]) {
		State->Args[1] = (ml_value_t *)Q;
		State->Base.run = (ml_state_fn)ml_polynomial_term_run;
		return ml_call(State, AddMethod, 2, State->Args);
	} else {
		State->Args[0] = (ml_value_t *)Q;
		return ml_polynomial_call_term(State);
	}
}

static void ml_polynomial_term_run(ml_polynomial_call_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[0] = Value;
	return ml_polynomial_call_term(State);
}

static void ml_polynomial_compute_powers(ml_polynomial_call_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_substitution_t *Sub = State->Subs->Values[State->I1];
	int I2 = State->I2;
	Sub->Values[I2] = Value;
	++I2;
	if (I2 < Sub->Degree) {
		State->I2 = I2;
		State->Args[0] = Sub->Values[0];
		State->Args[1] = Value;
		return ml_call(State, MulMethod, 2, State->Args);
	}
	for (int I1 = State->I1 + 1; I1 < State->Subs->Size; ++I1) {
		ml_substitution_t *Sub = State->Subs->Values[I1];
		if (Sub && Sub->Degree > 1) {
			State->I1 = I1;
			State->I2 = 1;
			State->Args[0] = Sub->Values[0];
			State->Args[1] = Sub->Values[0];
			State->Base.run = (ml_state_fn)ml_polynomial_compute_powers;
			return ml_call(State, MulMethod, 2, State->Args);
		}
	}
	State->I1 = 0;
	State->Args[0] = NULL;
	return ml_polynomial_call_term(State);
}

static void ml_polynomial_call(ml_state_t *Caller, ml_polynomial_t *P, int Count, ml_value_t **Args) {
	if (!Count) ML_RETURN(P);
	if (!P->Count) ML_RETURN(ml_real(0));
	if (ml_typeof(Args[0]) == MLNamesT) {
		ml_polynomial_call_state_t *State = xnew(ml_polynomial_call_state_t, Count, ml_substitution_t);
		int I = 0;
		ML_NAMES_FOREACH(Args[0], Iter) {
			int Variable = (intptr_t)stringmap_search(Variables, ml_string_value(Iter->Value));
			if (Variable) {
				int MaxDegree = 0;
				for (int J = 0; J < P->Count; ++J) {
					const ml_factors_t *F = P->Terms[J].Factors;
					for (int K = 0; K < F->Count; ++K) {
						if (F->Factors[K].Variable == Variable) {
							if (MaxDegree < F->Factors[K].Degree) MaxDegree = F->Factors[K].Degree;
						}
					}
				}
				if (MaxDegree) {
					ml_substitution_t *Sub = xnew(ml_substitution_t, MaxDegree, ml_value_t *);
					Sub->Variable = Variable;
					Sub->Degree = MaxDegree;
					Sub->Values[0] = ml_deref(Args[I + 1]);
					inthash_insert(State->Subs, Variable, Sub);
				}
			}
			++I;
		}
		if (!State->Subs->Size) ML_RETURN(P);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->P = P;
		for (int I1 = 0; I1 < State->Subs->Size; ++I1) {
			ml_substitution_t *Sub = State->Subs->Values[I1];
			if (Sub && Sub->Degree > 1) {
				State->I1 = I1;
				State->I2 = 1;
				State->Args[0] = Sub->Values[0];
				State->Args[1] = Sub->Values[0];
				State->Base.run = (ml_state_fn)ml_polynomial_compute_powers;
				return ml_call(State, MulMethod, 2, State->Args);
			}
		}
		State->I1 = 0;
		State->Args[0] = NULL;
		return ml_polynomial_call_term(State);
	} else {
		ML_ERROR("ImplementationError", "Not implemented yet");
	}
}

ML_TYPE(MLPolynomialT, (), "polynomial",
	.call = (void *)ml_polynomial_call
);

static ml_polynomial_t *ml_polynomial_const(double Value) {
	ml_polynomial_t *C = xnew(ml_polynomial_t, 1, ml_term_t);
	C->Type = MLPolynomialT;
	C->Count = 1;
	C->Terms->Coeff = Value;
	C->Terms->Factors = Constant;
	return C;
}

static ml_polynomial_t *ml_polynomial_add(const ml_polynomial_t *A, const ml_polynomial_t *B) {
	ml_polynomial_t *C = xnew(ml_polynomial_t, A->Count + B->Count, ml_term_t);
	C->Type = MLPolynomialT;
	const ml_term_t *TA = A->Terms, *TB = B->Terms;
	ml_term_t *TC = C->Terms;
	int CA = A->Count, CB = B->Count;
	while (CA && CB) {
		int Cmp = ml_factors_cmp(TA->Factors, TB->Factors);
		if (Cmp < 0) {
			*TC++ = *TB++;
			--CB;
		} else if (Cmp > 0) {
			*TC++ = *TA++;
			--CA;
		} else {
			double Coeff = TA->Coeff + TB->Coeff;
			if (fabs(Coeff) >= DBL_EPSILON) {
				TC->Coeff = Coeff;
				TC->Factors = TA->Factors;
				++TC;
			}
			++TA; ++TB;
			--CA; --CB;
		}
	}
	while (CA) {
		*TC++ = *TA++;
		--CA;
	}
	while (CB) {
		*TC++ = *TB++;
		--CB;
	}
	C->Count = TC - C->Terms;
	return C;
}

static ml_polynomial_t *ml_polynomial_sub(const ml_polynomial_t *A, const ml_polynomial_t *B) {
	ml_polynomial_t *C = xnew(ml_polynomial_t, A->Count + B->Count, ml_term_t);
	C->Type = MLPolynomialT;
	const ml_term_t *TA = A->Terms, *TB = B->Terms;
	ml_term_t *TC = C->Terms;
	int CA = A->Count, CB = B->Count;
	while (CA && CB) {
		int Cmp = ml_factors_cmp(TA->Factors, TB->Factors);
		if (Cmp < 0) {
			TC->Factors = TB->Factors;
			TC->Coeff = -TB->Coeff;
			++TC;
			++TB;
			--CB;
		} else if (Cmp > 0) {
			*TC++ = *TA++;
			--CA;
		} else {
			double Coeff = TA->Coeff - TB->Coeff;
			if (fabs(Coeff) >= DBL_EPSILON) {
				TC->Coeff = Coeff;
				TC->Factors = TA->Factors;
				++TC;
			}
			++TA; ++TB;
			--CA; --CB;
		}
	}
	while (CA) {
		*TC++ = *TA++;
		--CA;
	}
	while (CB) {
		TC->Factors = TB->Factors;
		TC->Coeff = -TB->Coeff;
		++TB;
		++TC;
		--CB;
	}
	C->Count = TC - C->Terms;
	return C;
}

static void ml_terms_sort(ml_term_t *Lo, ml_term_t *Hi) {
	ml_term_t *A = Lo, *B = Hi;
	ml_term_t P = *A, T = *B;
	while (A < B) {
		int Cmp = ml_factors_cmp(P.Factors, T.Factors);
		if (Cmp < 0) {
			*A++ = T;
			T = *A;
		} else if (Cmp > 0) {
			*B-- = T;
			T = *B;
		} else {
			P.Coeff += T.Coeff;
			T.Coeff = 0;
			*B-- = T;
			T = *B;
		}
	}
	*A = P;
	if (Lo < A - 1) ml_terms_sort(Lo, A - 1);
	if (B + 1 < Hi) ml_terms_sort(B + 1, Hi);
}

static ml_polynomial_t *ml_polynomial_mul(const ml_polynomial_t *A, const ml_polynomial_t *B) {
	int Count = A->Count * B->Count;
	ml_polynomial_t *C = xnew(ml_polynomial_t, Count, ml_term_t);
	C->Type = MLPolynomialT;
	const ml_term_t *TA = A->Terms, *TB = B->Terms;
	ml_term_t *TC = C->Terms;
	int CA = A->Count, CB = B->Count;
	for (int IA = 0; IA < CA; ++IA) for (int IB = 0; IB < CB; ++IB) {
		TC->Coeff = TA[IA].Coeff * TB[IB].Coeff;
		TC->Factors = ml_factors_mul(TA[IA].Factors, TB[IB].Factors);
		++TC;
	}
	ml_terms_sort(C->Terms, TC - 1);
	TC = C->Terms;
	ml_term_t *TC2 = TC;
	for (int I = 0; I < Count; ++I, ++TC2) if (TC2->Coeff) *TC++ = *TC2;
	C->Count = TC - C->Terms;
	return C;
}

static int ml_term_div(ml_term_t *A, ml_term_t *B, ml_term_t *C) {
	const ml_factor_t *FA = A->Factors->Factors, *FB = B->Factors->Factors;
	int CA = A->Factors->Count, CB = B->Factors->Count;
	ml_factors_t *Factors = xnew(ml_factors_t, CA, ml_factor_t);
	ml_factor_t *FC = Factors->Factors;
	int Degree = 0;
	while (CA && CB) {
		if (FA->Variable > FB->Variable) return 0;
		if (FA->Variable < FB->Variable) {
			Degree += FA->Degree;
			*FC++ = *FA++; --CA;
		} else {
			int D = FA->Degree - FB->Degree;
			if (D < 0) return 0;
			if (D > 0) {
				Degree += D;
				FC->Degree = D;
				FC->Variable = FA->Variable;
				++FC;
			}
			++FA; ++FB;
			--CA; --CB;
		}
	}
	if (CB) return 0;
	while (CA) {
		Degree += FA->Degree;
		*FC++ = *FA++;
		--CA;
	}
	if (Degree) {
		Factors->Degree = Degree;
		Factors->Count = FC - Factors->Factors;
		C->Factors = Factors;
	} else {
		C->Factors = Constant;
	}
	C->Coeff = A->Coeff / B->Coeff;
	return 1;
}

typedef struct {
	ml_polynomial_t *Q, *R;
} ml_quotient_t;

static ml_quotient_t ml_polynomial_quotient(ml_polynomial_t *A, ml_polynomial_t *B) {
	// TODO: optimize this code to prevent unneccessary allocations
	ml_polynomial_t *D = xnew(ml_polynomial_t, 1, ml_term_t);
	D->Type = MLPolynomialT;
	D->Count = 1;
	ml_polynomial_t *Q = NULL;
	for (int I = 0; I < A->Count;) {
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_printf(Buffer, "A = (");
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, "), B = (");
		ml_polynomial_write(Buffer, B);
		ml_stringbuffer_printf(Buffer, "), Q = (");
		if (Q) ml_polynomial_write(Buffer, Q);
		ml_stringbuffer_printf(Buffer, ")");
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		if (ml_term_div(A->Terms + I, B->Terms, D->Terms)) {
			if (Q) {
				Q = ml_polynomial_add(Q, D);
			} else {
				Q = xnew(ml_polynomial_t, 1, ml_term_t);
				Q->Type = MLPolynomialT;
				Q->Count = 1;
				Q->Terms[0] = D->Terms[0];
			}
			A = ml_polynomial_sub(A, ml_polynomial_mul(B, D));
		} else {
			++I;
		}
	}
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "A = (");
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, "), B = (");
	ml_polynomial_write(Buffer, B);
	ml_stringbuffer_printf(Buffer, "), Q = (");
	if (Q) ml_polynomial_write(Buffer, Q);
	ml_stringbuffer_printf(Buffer, ")");
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	return (ml_quotient_t){Q, A->Count ? A : NULL};
}

static ml_polynomial_t *ml_polynomial_spol(ml_polynomial_t *A, ml_polynomial_t *B) {
	int CA = A->Terms[0].Factors->Count;
	int CB = B->Terms[0].Factors->Count;
	const ml_factor_t *FA = A->Terms[0].Factors->Factors;
	const ml_factor_t *FB = B->Terms[0].Factors->Factors;
	ml_factors_t *MA = xnew(ml_factors_t, CB, ml_factor_t);
	ml_factors_t *MB = xnew(ml_factors_t, CA, ml_factor_t);
	ml_factor_t *MFA = MA->Factors, *MFB = MB->Factors;
	while (CA && CB) {
		if (FA->Variable < FB->Variable) {
			MB->Degree += FA->Degree;
			*MFB++ = *FA++;
			--CA;
		} else if (FA->Variable > FB->Variable) {
			MA->Degree += FB->Degree;
			*MFA++ = *FB++;
			--CB;
		} else if (FA->Degree > FB->Degree) {
			MFB->Variable = FA->Variable;
			MFB->Degree = FA->Degree - FB->Degree;
			MB->Degree += MFB->Degree;
			++MFB;
			++FA; ++FB;
			--CA; --CB;
		} else if (FA->Degree < FB->Degree) {
			MFA->Variable = FB->Variable;
			MFA->Degree = FB->Degree - FA->Degree;
			MA->Degree += MFA->Degree;
			++MFA;
			++FA; ++FB;
			--CA; --CB;
		} else {
			++FA; ++FB;
			--CA; --CB;
		}
	}
	while (CA) {
		MB->Degree += FA->Degree;
		*MFB++ = *FA++;
		--CA;
	}
	while (CB) {
		MA->Degree += FB->Degree;
		*MFA++ = *FB++;
		--CB;
	}
	MA->Count = MFA - MA->Factors;
	MB->Count = MFB - MB->Factors;
	double X = B->Terms[0].Coeff / A->Terms[0].Coeff;
	ml_polynomial_t *XA = xnew(ml_polynomial_t, A->Count, ml_term_t);
	XA->Type = MLPolynomialT;
	XA->Count = A->Count;
	for (int I = 0; I < A->Count; ++I) {
		XA->Terms[I].Coeff = A->Terms[I].Coeff * X;
		XA->Terms[I].Factors = ml_factors_mul(A->Terms[I].Factors, MA);
	}
	ml_terms_sort(XA->Terms, XA->Terms + XA->Count - 1);
	ml_polynomial_t *XB = xnew(ml_polynomial_t, B->Count, ml_term_t);
	XB->Type = MLPolynomialT;
	XB->Count = B->Count;
	for (int I = 0; I < B->Count; ++I) {
		XB->Terms[I].Coeff = B->Terms[I].Coeff;
		XB->Terms[I].Factors = ml_factors_mul(B->Terms[I].Factors, MB);
	}
	ml_terms_sort(XB->Terms, XB->Terms + XB->Count - 1);
	return ml_polynomial_sub(XA, XB);
}

static ml_polynomial_t *ml_polynomial_reduce(ml_polynomial_t *A, ml_polynomial_t *B) {
	for (int I = 0; I < A->Count; ++I) {
		int CA = A->Terms[I].Factors->Count;
		int CB = B->Terms[0].Factors->Count;
		const ml_factor_t *FA = A->Terms[I].Factors->Factors;
		const ml_factor_t *FB = B->Terms[0].Factors->Factors;
		ml_factors_t *MB = xnew(ml_factors_t, CA, ml_factor_t);
		ml_factor_t *MFB = MB->Factors;
		while (CA && CB) {
			if (FA->Variable < FB->Variable) {
				MB->Degree += FA->Degree;
				*MFB++ = *FA++;
				--CA;
			} else if (FA->Variable > FB->Variable) {
				goto next;
			} else if (FA->Degree > FB->Degree) {
				MFB->Variable = FA->Variable;
				MFB->Degree = FA->Degree - FB->Degree;
				MB->Degree += MFB->Degree;
				++MFB;
				++FA; ++FB;
				--CA; --CB;
			} else if (FA->Degree < FB->Degree) {
				goto next;
			} else {
				++FA; ++FB;
				--CA; --CB;
			}
		}
		if (CB) goto next;
		while (CA) {
			MB->Degree += FA->Degree;
			*MFB++ = *FA++;
			--CA;
		}
		MB->Count = MFB - MB->Factors;
		ml_polynomial_t *XB = xnew(ml_polynomial_t, B->Count, ml_term_t);
		XB->Type = MLPolynomialT;
		XB->Count = B->Count;
		double X = A->Terms[I].Coeff / B->Terms[0].Coeff;
		for (int I = 0; I < B->Count; ++I) {
			XB->Terms[I].Coeff = X * B->Terms[I].Coeff;
			XB->Terms[I].Factors = ml_factors_mul(B->Terms[I].Factors, MB);
		}
		ml_terms_sort(XB->Terms, XB->Terms + XB->Count - 1);
		return ml_polynomial_sub(A, XB);
	next:;
	}
	return NULL;
}

ML_METHOD(MLPolynomialT, MLStringT) {
	const char *Name = ml_string_value(Args[0]);
	int *Slot = (int *)stringmap_slot(Variables, Name);
	if (!Slot[0]) {
		int Index = Slot[0] = Variables->Size;
		Names = GC_realloc(Names, Index * sizeof(const char *));
		Names[Index - 1] = Name;
	}
	ml_factors_t *Factors = xnew(ml_factors_t, 1, ml_factor_t);
	Factors->Degree = 1;
	Factors->Count = 1;
	Factors->Factors->Variable = Slot[0];
	Factors->Factors->Degree = 1;
	ml_polynomial_t *Poly = xnew(ml_polynomial_t, 1, ml_term_t);
	Poly->Type = MLPolynomialT;
	Poly->Count = 1;
	Poly->Terms->Factors = Factors;
	Poly->Terms->Coeff = 1;
	return (ml_value_t *)Poly;
}

ML_METHOD("+", MLPolynomialT, MLNumberT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	double N = ml_real_value(Args[1]);
	if (fabs(N) < DBL_EPSILON) return (ml_value_t *)A;
	int CA = A->Count;
	if (A->Terms[CA - 1].Factors->Count == 0) {
		double Sum = A->Terms[CA - 1].Coeff + N;
		if (fabs(Sum) > DBL_EPSILON) {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA;
			memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
			B->Terms[CA - 1].Coeff = Sum;
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_write(Buffer, "(", 1);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") + (%g) = ", N);
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		} else {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA - 1, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA - 1;
			memcpy(B->Terms, A->Terms, (CA - 1) * sizeof(ml_term_t));
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_write(Buffer, "(", 1);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") + (%g) = ", N);
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		}
	} else {
		ml_polynomial_t *B = xnew(ml_polynomial_t, CA + 1, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = CA + 1;
		memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
		B->Terms[CA].Factors = Constant;
		B->Terms[CA].Coeff = N;
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_write(Buffer, "(", 1);
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, ") + (%g) = ", N);
		ml_polynomial_write(Buffer, B);
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		return (ml_value_t *)B;
	}
}

ML_METHOD("+", MLNumberT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[1];
	double N = ml_real_value(Args[0]);
	if (fabs(N) < DBL_EPSILON) return (ml_value_t *)A;
	int CA = A->Count;
	if (A->Terms[CA - 1].Factors->Count == 0) {
		double Sum = A->Terms[CA - 1].Coeff + N;
		if (fabs(Sum) > DBL_EPSILON) {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA;
			memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
			B->Terms[CA - 1].Coeff = Sum;
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_printf(Buffer, "(%g) + (", N);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") = ");
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		} else {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA - 1, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA - 1;
			memcpy(B->Terms, A->Terms, (CA - 1) * sizeof(ml_term_t));
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_printf(Buffer, "(%g) + (", N);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") = ");
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		}
	} else {
		ml_polynomial_t *B = xnew(ml_polynomial_t, CA + 1, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = CA + 1;
		memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
		B->Terms[CA].Factors = Constant;
		B->Terms[CA].Coeff = N;
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_printf(Buffer, "(%g) + (", N);
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, ") = ");
		ml_polynomial_write(Buffer, B);
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		return (ml_value_t *)B;
	}
}

ML_METHOD("+", MLPolynomialT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	ml_polynomial_t *C = ml_polynomial_add(A, B);
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "(");
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") + (");
	ml_polynomial_write(Buffer, B);
	ml_stringbuffer_printf(Buffer, ") = ");
	ml_polynomial_write(Buffer, C);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	if (!C->Count) return ml_real(0);
	if (C->Count == 1 && C->Terms->Factors->Count == 0) return ml_real(C->Terms->Coeff);
	return (ml_value_t *)C;
}

ML_METHOD("-", MLPolynomialT, MLNumberT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	double N = ml_real_value(Args[1]);
	if (fabs(N) < DBL_EPSILON) return (ml_value_t *)A;
	int CA = A->Count;
	if (A->Terms[CA - 1].Factors->Count == 0) {
		double Sum = A->Terms[CA - 1].Coeff - N;
		if (fabs(Sum) > DBL_EPSILON) {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA;
			memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
			B->Terms[CA - 1].Coeff = Sum;
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_write(Buffer, "(", 1);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") - (%g) = ", N);
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		} else {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA - 1, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA - 1;
			memcpy(B->Terms, A->Terms, (CA - 1) * sizeof(ml_term_t));
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_write(Buffer, "(", 1);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") - (%g) = ", N);
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		}
	} else {
		ml_polynomial_t *B = xnew(ml_polynomial_t, CA + 1, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = CA + 1;
		memcpy(B->Terms, A->Terms, CA * sizeof(ml_term_t));
		B->Terms[CA].Factors = Constant;
		B->Terms[CA].Coeff = -N;
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_write(Buffer, "(", 1);
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, ") - (%g) = ", N);
		ml_polynomial_write(Buffer, B);
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		return (ml_value_t *)B;
	}
}

ML_METHOD("-", MLNumberT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[1];
	double N = ml_real_value(Args[0]);
	int CA = A->Count;
	if (fabs(N) < DBL_EPSILON) {
		ml_polynomial_t *B = xnew(ml_polynomial_t, CA, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = CA;
		for (int I = 0; I < CA; ++I) {
			B->Terms[I].Factors = A->Terms[I].Factors;
			B->Terms[I].Coeff = -A->Terms[I].Coeff;
		}
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_printf(Buffer, "(%g) - (", N);
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, ") = ");
		ml_polynomial_write(Buffer, B);
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		return (ml_value_t *)B;
	} else if (A->Terms[CA - 1].Factors->Count == 0) {
		double Sum = N - A->Terms[CA - 1].Coeff;
		if (fabs(Sum) > DBL_EPSILON) {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA;
			for (int I = 0; I < CA; ++I) {
				B->Terms[I].Factors = A->Terms[I].Factors;
				B->Terms[I].Coeff = -A->Terms[I].Coeff;
			}
			B->Terms[CA - 1].Coeff = Sum;
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_printf(Buffer, "(%g) - (", N);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") = ");
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		} else {
			ml_polynomial_t *B = xnew(ml_polynomial_t, CA - 1, ml_term_t);
			B->Type = MLPolynomialT;
			B->Count = CA - 1;
			for (int I = 0; I < CA - 1; ++I) {
				B->Terms[I].Factors = A->Terms[I].Factors;
				B->Terms[I].Coeff = -A->Terms[I].Coeff;
			}
#ifdef ML_POLY_DEBUG
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			ml_stringbuffer_printf(Buffer, "(%g) - (", N);
			ml_polynomial_write(Buffer, A);
			ml_stringbuffer_printf(Buffer, ") = ");
			ml_polynomial_write(Buffer, B);
			puts(ml_stringbuffer_get_string(Buffer));
#endif
			return (ml_value_t *)B;
		}
	} else {
		ml_polynomial_t *B = xnew(ml_polynomial_t, CA + 1, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = CA + 1;
		for (int I = 0; I < CA; ++I) {
			B->Terms[I].Factors = A->Terms[I].Factors;
			B->Terms[I].Coeff = -A->Terms[I].Coeff;
		}
		B->Terms[CA].Factors = Constant;
		B->Terms[CA].Coeff = N;
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_printf(Buffer, "(%g) - (", N);
		ml_polynomial_write(Buffer, A);
		ml_stringbuffer_printf(Buffer, ") = ");
		ml_polynomial_write(Buffer, B);
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		return (ml_value_t *)B;
	}
}

ML_METHOD("-", MLPolynomialT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	ml_polynomial_t *C = ml_polynomial_sub(A, B);
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "(");
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") - (");
	ml_polynomial_write(Buffer, B);
	ml_stringbuffer_printf(Buffer, ") = ");
	ml_polynomial_write(Buffer, C);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	if (!C->Count) return ml_real(0);
	if (C->Count == 1 && C->Terms->Factors->Count == 0) return ml_real(C->Terms->Coeff);
	return (ml_value_t *)C;
}

ML_METHOD("*", MLPolynomialT, MLNumberT) {
	double N = ml_real_value(Args[1]);
	if (fabs(N) < DBL_EPSILON) return Args[1];
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	int CA = A->Count;
	ml_polynomial_t *B = xnew(ml_polynomial_t, (CA + 1), ml_term_t);
	B->Type = MLPolynomialT;
	B->Count = CA;
	for (int I = 0; I < CA; ++I) {
		B->Terms[I].Factors = A->Terms[I].Factors;
		B->Terms[I].Coeff = A->Terms[I].Coeff * N;
	}
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_write(Buffer, "(", 1);
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") * (%g) = ", N);
	ml_polynomial_write(Buffer, B);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	return (ml_value_t *)B;
}

ML_METHOD("*", MLNumberT, MLPolynomialT) {
	double N = ml_real_value(Args[0]);
	if (fabs(N) < DBL_EPSILON) return Args[0];
	ml_polynomial_t *A = (ml_polynomial_t *)Args[1];
	int CA = A->Count;
	ml_polynomial_t *B = xnew(ml_polynomial_t, (CA + 1), ml_term_t);
	B->Type = MLPolynomialT;
	B->Count = CA;
	for (int I = 0; I < CA; ++I) {
		B->Terms[I].Factors = A->Terms[I].Factors;
		B->Terms[I].Coeff = N * A->Terms[I].Coeff;
	}
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "(%g) * (", N);
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") = ");
	ml_polynomial_write(Buffer, B);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	return (ml_value_t *)B;
}

ML_METHOD("*", MLPolynomialT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	ml_polynomial_t *C = ml_polynomial_mul(A, B);
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "(");
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") * (");
	ml_polynomial_write(Buffer, B);
	ml_stringbuffer_printf(Buffer, ") = ");
	ml_polynomial_write(Buffer, C);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	return (ml_value_t *)C;
}

ML_METHOD("^", MLPolynomialT, MLIntegerT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	int N = ml_integer_value(Args[1]);
	if (N < 0) return ml_error("RangeError", "Negative powers not implemented yet");
	if (N == 0) return ml_real(1);
	if (A->Count == 1) {
		int C = A->Terms->Factors->Count;
		const ml_factors_t *FA = A->Terms->Factors;
		ml_factors_t *FB = xnew(ml_factors_t, C, ml_factor_t);
		FB->Count = C;
		FB->Degree = FA->Degree * N;
		memcpy(FB->Factors, FA->Factors, C * sizeof(ml_factor_t));
		for (int I = 0; I < C; ++I) FB->Factors[I].Degree *= N;
		ml_polynomial_t *B = xnew(ml_polynomial_t, 1, ml_term_t);
		B->Type = MLPolynomialT;
		B->Count = 1;
		B->Terms->Coeff = pow(A->Terms->Coeff, N);
		B->Terms->Factors = FB;
		return (ml_value_t *)B;
	}
	ml_polynomial_t *S = A, *B = NULL;
	while (N) {
		if (N % 2) B = B ? ml_polynomial_mul(B, S) : S;
		N /= 2;
		S = ml_polynomial_mul(S, S);
	}
	return (ml_value_t *)B;
}

ML_METHOD("d", MLPolynomialT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	if (B->Count != 1 || B->Terms[0].Coeff != 1 || B->Terms[0].Factors->Count != 1 || B->Terms[0].Factors->Degree != 1) {
		return ml_error("ValueError", "Invalid polynomial for derivative");
	}
	ml_polynomial_t *C = xnew(ml_polynomial_t, A->Count, ml_term_t);
	C->Type = MLPolynomialT;
	int Variable = B->Terms[0].Factors->Factors[0].Variable;
	const ml_term_t *TA = A->Terms;
	ml_term_t *TC = C->Terms;
	for (int I = A->Count; --I >= 0; ++TA) {
		int N = TA->Factors->Count;
		for (int J = 0; J < N; ++J) {
			const ml_factor_t *FA = TA->Factors->Factors + J;
			if (FA->Variable == Variable) {
				TC->Coeff = TA->Coeff * FA->Degree;
				if (FA->Degree == 1) {
					ml_factors_t *F = xnew(ml_factors_t, N - 1, ml_factor_t);
					for (int K = 0; K < J; ++K) F->Factors[K] = TA->Factors->Factors[K];
					for (int K = J + 1; K < N; ++K) F->Factors[K  - 1] = TA->Factors->Factors[K];
					F->Count = N - 1;
					F->Degree = TA->Factors->Degree - 1;
					TC->Factors = F;
				} else {
					ml_factors_t *F = xnew(ml_factors_t, N, ml_factor_t);
					for (int K = 0; K < N; ++K) F->Factors[K] = TA->Factors->Factors[K];
					--F->Factors[J].Degree;
					F->Count = N;
					F->Degree = TA->Factors->Degree - 1;
					TC->Factors = F;
				}
				++TC;
			}
		}
	}
	C->Count = TC - C->Terms;
	if (!C->Count) return ml_real(0);
	if (C->Count == 1 && C->Terms->Factors->Count == 0) return ml_real(C->Terms->Coeff);
	return (ml_value_t *)C;
}

ML_METHOD("append", MLStringBufferT, MLPolynomialT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_polynomial_t *Poly = (ml_polynomial_t *)Args[1];
	ml_polynomial_write(Buffer, Poly);
	return MLSome;
}

ML_METHOD("/", MLPolynomialT, MLNumberT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	double N = ml_real_value(Args[1]);
	int CA = A->Count;
	ml_polynomial_t *B = xnew(ml_polynomial_t, (CA + 1), ml_term_t);
	B->Type = MLPolynomialT;
	B->Count = CA;
	for (int I = 0; I < CA; ++I) {
		B->Terms[I].Factors = A->Terms[I].Factors;
		B->Terms[I].Coeff = A->Terms[I].Coeff / N;
	}
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_write(Buffer, "(", 1);
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, ") / (%g) = ", N);
	ml_polynomial_write(Buffer, B);
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	return (ml_value_t *)B;
}

ML_TYPE(MLPolynomialRationalT, (), "polynomial::rational");

static ml_value_t *ml_polynomial_div(ml_polynomial_t *A, ml_polynomial_t *B) {
	ml_polynomial_t *T = A, *G = B;
	int Cycle = 0;
	for (;;) {
#ifdef ML_POLY_DEBUG
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_printf(Buffer, "T = (");
		ml_polynomial_write(Buffer, T);
		ml_stringbuffer_printf(Buffer, "), G = (");
		ml_polynomial_write(Buffer, G);
		ml_stringbuffer_printf(Buffer, ")");
		puts(ml_stringbuffer_get_string(Buffer));
#endif
		ml_quotient_t D = ml_polynomial_quotient(T, G);
		if (!D.R) {
			if (G == B) {
				if (D.Q->Count == 1 && D.Q->Terms->Factors->Count == 0) {
					return ml_real(D.Q->Terms->Coeff);
				} else {
					return (ml_value_t *)D.Q;
				}
			} else {
				break;
			}
		} else if (!D.Q) {
			if (++Cycle == 2) {
				G = ml_polynomial_const(1);
				break;
			}
		} else {
			Cycle = 0;
		}
		T = G; G = D.R;
	}
#ifdef ML_POLY_DEBUG
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "A = (");
	ml_polynomial_write(Buffer, A);
	ml_stringbuffer_printf(Buffer, "), B = (");
	ml_polynomial_write(Buffer, B);
	ml_stringbuffer_printf(Buffer, "), G = (");
	ml_polynomial_write(Buffer, G);
	ml_stringbuffer_printf(Buffer, ")");
	puts(ml_stringbuffer_get_string(Buffer));
#endif
	ml_polynomial_rational_t *C = new(ml_polynomial_rational_t);
	C->Type = MLPolynomialRationalT;
	if (G->Count == 1 && G->Terms->Factors->Count == 0) {
		C->A = A;
		C->B = B;
	} else {
		C->A = ml_polynomial_quotient(A, G).Q;
		C->B = ml_polynomial_quotient(B, G).Q;
	}
	return (ml_value_t *)C;
}

ML_METHOD("/", MLNumberT, MLPolynomialT) {
	ml_polynomial_t *A = ml_polynomial_const(ml_real_value(Args[0]));
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(A, B);
}

ML_METHOD("/", MLPolynomialT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(A, B);
}

ML_METHOD("spol", MLPolynomialT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return (ml_value_t *)ml_polynomial_spol(A, B);
}

ML_METHOD("red", MLPolynomialT, MLPolynomialT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return (ml_value_t *)ml_polynomial_reduce(A, B) ?: MLNil;
}

ML_METHOD("+", MLNumberT, MLPolynomialRationalT) {
	ml_polynomial_t *A = ml_polynomial_const(ml_real_value(Args[0]));
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_add(ml_polynomial_mul(A, B->B), B->A), B->B);
}

ML_METHOD("+", MLPolynomialRationalT, MLNumberT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = ml_polynomial_const(ml_real_value(Args[1]));
	return ml_polynomial_div(ml_polynomial_add(A->A, ml_polynomial_mul(A->B, B)), A->B);
}

ML_METHOD("+", MLPolynomialT, MLPolynomialRationalT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_add(ml_polynomial_mul(A, B->B), B->A), B->B);
}

ML_METHOD("+", MLPolynomialRationalT, MLPolynomialT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_add(A->A, ml_polynomial_mul(A->B, B)), A->B);
}

ML_METHOD("+", MLPolynomialRationalT, MLPolynomialRationalT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(
		ml_polynomial_add(ml_polynomial_mul(A->A, B->B), ml_polynomial_mul(A->B, B->A)),
		ml_polynomial_mul(A->B, B->B)
	);
}

ML_METHOD("-", MLNumberT, MLPolynomialRationalT) {
	ml_polynomial_t *A = ml_polynomial_const(ml_real_value(Args[0]));
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_sub(ml_polynomial_mul(A, B->B), B->A), B->B);
}

ML_METHOD("-", MLPolynomialRationalT, MLNumberT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = ml_polynomial_const(ml_real_value(Args[1]));
	return ml_polynomial_div(ml_polynomial_sub(A->A, ml_polynomial_mul(A->B, B)), A->B);
}

ML_METHOD("-", MLPolynomialT, MLPolynomialRationalT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_sub(ml_polynomial_mul(A, B->B), B->A), B->B);
}

ML_METHOD("-", MLPolynomialRationalT, MLPolynomialT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_sub(A->A, ml_polynomial_mul(A->B, B)), A->B);
}

ML_METHOD("-", MLPolynomialRationalT, MLPolynomialRationalT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(
		ml_polynomial_sub(ml_polynomial_mul(A->A, B->B), ml_polynomial_mul(A->B, B->A)),
		ml_polynomial_mul(A->B, B->B)
	);
}

ML_METHOD("*", MLNumberT, MLPolynomialRationalT) {
	ml_polynomial_t *A = ml_polynomial_const(ml_real_value(Args[0]));
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_mul(A, B->A), B->B);
}

ML_METHOD("*", MLPolynomialRationalT, MLNumberT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = ml_polynomial_const(ml_real_value(Args[1]));
	return ml_polynomial_div(ml_polynomial_mul(A->A, B), A->B);
}

ML_METHOD("*", MLPolynomialT, MLPolynomialRationalT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_mul(A, B->A), B->B);
}

ML_METHOD("*", MLPolynomialRationalT, MLPolynomialT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_mul(A->A, B), A->B);
}

ML_METHOD("*", MLPolynomialRationalT, MLPolynomialRationalT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(
		ml_polynomial_mul(A->A, B->A),
		ml_polynomial_mul(A->B, B->B)
	);
}

ML_METHOD("/", MLNumberT, MLPolynomialRationalT) {
	ml_polynomial_t *A = ml_polynomial_const(ml_real_value(Args[0]));
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_mul(A, B->B), B->A);
}

ML_METHOD("/", MLPolynomialRationalT, MLNumberT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = ml_polynomial_const(ml_real_value(Args[1]));
	return ml_polynomial_div(A->A, ml_polynomial_mul(A->B, B));
}

ML_METHOD("/", MLPolynomialT, MLPolynomialRationalT) {
	ml_polynomial_t *A = (ml_polynomial_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(ml_polynomial_mul(A, B->B), B->A);
}

ML_METHOD("/", MLPolynomialRationalT, MLPolynomialT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_t *B = (ml_polynomial_t *)Args[1];
	return ml_polynomial_div(A->A, ml_polynomial_mul(A->B, B));
}

ML_METHOD("/", MLPolynomialRationalT, MLPolynomialRationalT) {
	ml_polynomial_rational_t *A = (ml_polynomial_rational_t *)Args[0];
	ml_polynomial_rational_t *B = (ml_polynomial_rational_t *)Args[1];
	return ml_polynomial_div(
		ml_polynomial_mul(A->A, B->B),
		ml_polynomial_mul(A->B, B->A)
	);
}

ML_METHOD("append", MLStringBufferT, MLPolynomialRationalT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_polynomial_rational_t *Rat = (ml_polynomial_rational_t *)Args[1];
	ml_stringbuffer_write(Buffer, "[", 1);
	ml_polynomial_write(Buffer, Rat->A);
	ml_stringbuffer_write(Buffer, " / ", 3);
	ml_polynomial_write(Buffer, Rat->B);
	ml_stringbuffer_write(Buffer, "]", 1);
	return MLSome;
}

void ml_polynomial_init(stringmap_t *Globals) {
#include "ml_polynomial_init.c"
	if (Globals) {
		stringmap_insert(Globals, "polynomial", MLPolynomialT);
	}
}
