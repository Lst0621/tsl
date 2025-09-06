// Functions for polynomials. Factors are listed in ascending order of degrees
import { get_sup } from "../util.js";
import { power_to_latex } from "./latex.js";
function join_terms(terms) {
    if (terms.length == 0) {
        return "0";
    }
    let result = "";
    for (let i = 0; i < terms.length; i++) {
        let term = terms[i];
        if (term.length == 0) {
            continue;
        }
        if (i == 0) {
            result += term;
            continue;
        }
        if (!term.startsWith("-")) {
            result += "+";
        }
        result += term;
    }
    return result;
}
function factor_to_string_for_non_const(factor) {
    if (factor == -1) {
        return "-";
    }
    if (factor == 1) {
        return "";
    }
    return factor.toString();
}
function get_latex_term(indeterminate, factor, degree) {
    if (factor == 0) {
        return "";
    }
    if (degree == 0) {
        return factor.toString();
    }
    if (degree == 1) {
        return factor_to_string_for_non_const(factor) + indeterminate;
    }
    return factor_to_string_for_non_const(factor) + power_to_latex(indeterminate, degree.toString());
}
export function poly_to_latex_low_to_high(poly) {
    let indeterminate = 'x';
    let terms = [];
    for (let degree = 0; degree < poly.length; degree++) {
        let factor = poly[degree];
        terms.push(get_latex_term(indeterminate, factor, degree));
    }
    return join_terms(terms);
}
export function poly_to_latex_high_to_low(poly) {
    let indeterminate = 'x';
    let terms = [];
    for (let degree = 0; degree < poly.length; degree++) {
        let factor = poly[degree];
        terms.push(get_latex_term(indeterminate, factor, degree));
    }
    return join_terms(terms.reverse());
}
export function poly_to_html(poly) {
    let indeterminate = 'x';
    let terms = [];
    for (let degree = 0; degree < poly.length; degree++) {
        let factor = poly[degree];
        if (factor == 0) {
            continue;
        }
        if (degree == 0) {
            terms.push(factor.toString());
            continue;
        }
        if (degree == 1) {
            terms.push(factor_to_string_for_non_const(factor) + indeterminate);
            continue;
        }
        terms.push(factor_to_string_for_non_const(factor) + indeterminate + get_sup(degree.toString()));
    }
    return join_terms(terms);
}
export function poly_eval(poly, x) {
    let powers = 1;
    let sum = 0;
    for (let degree = 0; degree < poly.length; degree++) {
        let factor = poly[degree];
        sum += factor * powers;
        powers *= x;
    }
    return sum;
}
