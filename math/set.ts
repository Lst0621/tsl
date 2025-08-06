import {matrix_multiply_general, transpose} from "./matrix.js";

export function is_reflexive(relation: boolean[][]) {
    let len = relation.length
    for (let i = 0; i < len; i++) {
        if (!relation[i][i]) {
            return false
        }
    }
    return true
}

export function is_symmetric(relation: boolean[][]) {
    let len = relation.length
    for (let i = 0; i < len; i++) {
        for (let j = i + 1; j < relation.length; j++) {
            if (relation[j][i] != relation[i][j]) {
                return false
            }
        }
    }
    return true
}

export function is_antisymmetric(relation: boolean[][]) {
    let len = relation.length
    for (let i = 0; i < len; i++) {
        for (let j = i + 1; j < relation.length; j++) {
            if (relation[j][i] && relation[i][j]) {
                return false
            }
        }
    }
    return true
}

export function is_transitive(relation: boolean[][]) {
    let len = relation.length
    for (let i = 0; i < relation.length; i++) {
        for (let j = 0; j < relation.length; j++) {
            if (!relation[i][j]) {
                continue
            }
            for (let k = 0; k < relation.length; k++) {
                if (relation[j][k]) {
                    if (!relation[i][k]) {
                        return false
                    }
                }
            }
        }
    }
    return true
}

export function is_equivalence(relation: boolean[][]) {
    return is_reflexive(relation) && is_symmetric(relation) && is_transitive(relation)
}

export function cartesian_product<T>(inputs: T[][]): T[][] {
    return cartesian_product_matrix(inputs)
}

function cartesian_product_backtrack<T>(inputs: T[][]): T[][] {
    let all_combinations: T[][] = []
    cartesian_helper(inputs, [], all_combinations)
    return all_combinations;
}

function cartesian_helper<T>(inputs: T[][], l: T[], all_combinations: T[][]) {
    let index: number = l.length;
    let number_of_sets: number = inputs.length;
    if (number_of_sets == index) {
        all_combinations.push(Array.from(l));
        return;
    }
    for (let element of inputs[index]) {
        l.push(element)
        cartesian_helper(inputs, l, all_combinations)
        l.pop()
    }
}

function cartesian_product_matrix<T>(inputs: T[][]) {
    let len = inputs.length;
    if (len == 0) {
        return inputs;
    }
    let result: T[][] = [[]]
    for (let i = 0; i < len; i++) {
        if (inputs[i].length == 0) {
            console.log("input " + i.toString() + " is empty!")
            return []
        }

        result = matrix_multiply_general(
            transpose([result]),
            [inputs[i]],
            (a: T[], b: T) => ([...Array.from(a), b]),
            (a: T[], b: T[]) => a
        ).flat()
    }
    // need es2019 for flat
    return result
}