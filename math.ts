import {get_sup} from "./util.js";

export function gcd(a_in: number, b_in: number): number {
    let a: number = Math.abs(a_in);
    let b: number = Math.abs(b_in);
    if (a == 0 || b == 0) {
        return a + b
    }

    if (a == b) {
        return a
    }

    if (a > b) {
        let tmp: number = a
        a = b
        b = tmp
    }

    // a < b
    while (true) {
        let res = b % a
        if (res == 0) {
            return a
        }
        b = a
        a = res
    }
}

export function multiply_mod_n(a: number, b: number, n: number): number {
    return a * b % n;
}

export function get_multiply_mod_n_function(n: number) {
    return (a: number, b: number) => multiply_mod_n(a, b, n)
}

export function add_mod_n(a: number, b: number, n: number): number {
    return (a + b) % n;
}

export function get_add_mod_n_function(n: number) {
    return (a: number, b: number) => add_mod_n(a, b, n)
}

export function add_inverse_mod_n(a: number, n: number) {
    return (n - (a % n)) % n;
}

export function get_add_inverse_mod_n_function(n: number) {
    return (a: number) => add_inverse_mod_n(a, n)
}

export function mul_inverse_mod_n(a: number, n: number) {
    // ax=1 (mod n)
    if (gcd(a, n) != 1) {
        throw Error("No multiplication inverse exists for " + a + " mod " + n);
    }
    for (let i = 1; i < n; i++) {
        if (multiply_mod_n(i, a, n) == 1) {
            return i;
        }
    }
    throw Error("No multiplication inverse exists for " + a + " mod " + n);
}

export function get_mul_inverse_mod_n_function(n: number) {
    return (a: number) => mul_inverse_mod_n(a, n)
}

export function are_co_prime(a: number, b: number): boolean {
    return gcd(a, b) == 1
}

export function totient(n: number): number {
    let ans: number = 0
    for (let i = 1; i <= n; i++) {
        if (are_co_prime(i, n)) {
            ans += 1
        }
    }
    return ans
}

export function get_u_n(n: number): number[] {
    let u: number[] = []
    for (let i = 1; i <= n; i++) {
        if (are_co_prime(i, n)) {
            u.push(i)
        }
    }
    return u
}

export function get_primitive_roots(n: number) {
    if (n == 1) {
        return [1]
    }
    let u = get_u_n(n)
    let ret = []
    for (let i = 0; i < u.length; i++) {
        let a = u[i]
        let g = generate_group([a], get_multiply_mod_n_function(n), (a, b) => a == b, n)
        if (g.length == u.length) {
            ret.push(a)
        }
    }
    return ret
}

export function pad_permutations(p1: number[], p2: number[]) {
    let l1 = p1.length;
    let l2 = p2.length;
    let l = Math.max(l1, l2);
    let p1_copy = Array.from(p1)
    let p2_copy = Array.from(p2)

    // fill
    for (let i = l1 + 1; i <= l; i++) {
        p1_copy.push(i)
    }
    for (let i = l2 + 1; i <= l; i++) {
        p2_copy.push(i)
    }
    return [p1_copy, p2_copy]
}

export function array_eq<T>(a: T[], b: T[]): boolean {
    let len_a: number = a.length;
    let len_b: number = b.length;
    if (len_a != len_b) {
        return false;
    }
    for (let i = 0; i < len_a; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

export function array_eq_2d<T>(a: T[][], b: T[][]): boolean {
    let len_a: number = a.length;
    let len_b: number = b.length;
    if (len_a != len_b) {
        return false;
    }
    for (let i = 0; i < len_a; i++) {
        if (!array_eq(a[i], b[i])) {
            return false;
        }
    }
    return true;
}

export function permutation_eq(p1: number[], p2: number[]) {
    let l1 = p1.length;
    let l2 = p2.length;
    let l = Math.max(l1, l2);

    let padded = pad_permutations(p1, p2)
    let p1_copy = padded[0]
    let p2_copy = padded[1]
    return array_eq(p1_copy, p2_copy)
}

export function permutation_multiply(p1: number[], p2: number[]): number[] {
    let l1 = p1.length;
    let l2 = p2.length;
    let l = Math.max(l1, l2);

    let padded = pad_permutations(p1, p2)
    let p1_copy = padded[0]
    let p2_copy = padded[1]

    let ans: number[] = []
    for (let i = 1; i <= l; i++) {
        let p2_i = p2_copy[i - 1]
        let p1_p2_i: number = p1_copy[p2_i - 1]
        ans.push(p1_p2_i)
    }
    return ans
}

export function get_identity_permutation(n: number): number[] {
    return Array(n).fill(0).map((_, i) => i + 1);
}

export function get_all_permutations(n: number): number[][] {
    let e: number[] = get_identity_permutation(n)
    let perm: number[] = e
    let ans: number[][] = [e]
    while (true) {
        let next: number[] = next_permutation(perm)
        if (next.every((val, i) => val === e[i])) {
            break
        }
        perm = next
        ans.push(perm)
    }
    return ans
}

export function get_all_permutations_recursive(n: number): number[][] {
    if (n == 1) {
        return [[1]]
    }
    let per_n_minus_one = get_all_permutations_recursive(n - 1)
    let ans: number[][] = []
    for (let permutation of per_n_minus_one) {
        for (let i: number = 0; i <= n - 1; i++) {
            let cp: number[] = Array.from(permutation)
            cp.splice(n - 1 - i, 0, n)
            ans.push(cp)
        }
    }
    return ans
}

export function is_cycle_valid(cycle: number[]) {
    if (cycle == null) {
        return false
    }
    for (let i = 0; i < cycle.length; i++) {
        if (isNaN(cycle[i])) {
            return false
        }
        for (let j = i + 1; j < cycle.length; j++) {
            if (cycle[j] == cycle[i]) {
                return false
            }
        }
    }

    return true
}

export function get_permutation_from_cycle(cycle: number[]): number[] {
    if (cycle == null) {
        return []
    }
    let num_element: number = cycle[0]
    for (let num of cycle) {
        num_element = Math.max(num, num_element)
    }
    let perm: number[] = get_identity_permutation(num_element)
    if (cycle.length == 1) {
        return perm
    }
    for (let i = 0; i < cycle.length; i++) {
        let from = cycle[i]
        let to = (i + 1) < cycle.length ? cycle[i + 1] : cycle[0]
        perm[from - 1] = to
    }
    return perm
}

export function get_cycles_from_permutations(perm: number[]): number[][] {
    let visited = Array(perm.length).fill(0)
    let cycles: number[][] = []
    let cycle: number[] = []
    let i = 0
    while (true) {
        if (i == perm.length) {
            break
        }
        if (visited[i] == true) {
            if (cycle.length != 0) {
                cycles.push(Array.from(cycle))
            }
            cycle = []
            i = i + 1
            continue
        }
        let from: number = i + 1
        let to: number = perm[i]
        visited[i] = true
        cycle.push(from)
        i = to - 1
    }
    return cycles
}

export function get_permutation_parity(perm: number[]) {
    let cycles: number[][] = get_cycles_from_permutations(perm)
    let parity: boolean = true
    for (let cycle of cycles) {
        let is_cycle_odd = cycle.length % 2 == 0
        parity = (parity && !is_cycle_odd) || (!parity && is_cycle_odd)
    }
    return parity
}

export function get_arrow_string_from_cycle(cycle: number[]): string {
    let arrow = "->"
    let ret = "("
    ret += cycle.map(String).join(arrow)
    ret += arrow + cycle[0].toString() + ")"
    return ret
}

export function get_string_from_cycle(cycle: number[]): string {
    let join_str = cycle.some(value => value > 9) ? "," : ""
    return "(" + cycle.map(String).join(join_str) + ")"
}

export function per_to_arrow(perm: number[]): string {
    let cycles: number[][] = get_cycles_from_permutations(perm)
    let ret: string = cycles.filter((cycle) => cycle.length > 1).map(get_arrow_string_from_cycle).join("")
    if (ret.length == 0) {
        ret = "e"
    }
    return ret
}

export function perm_to_str(perm: number[]) {
    let cycles: number[][] = get_cycles_from_permutations(perm)
    let cycle_str: string = cycles.filter((cycle) => cycle.length > 1).map(get_string_from_cycle).join("")
    if (cycle_str.length == 0) {
        return "e"
    }
    return cycle_str
}

function next_permutation(perm: number[]): number[] {
    let next: number[] = Array.from(perm)
    next_permutation_in_place(next)
    return next
}

function next_permutation_in_place(nums: number[]): void {
    // Step 1: Find the first index `i` from the end where nums[i] < nums[i + 1]
    let i = nums.length - 2;
    while (i >= 0 && nums[i] >= nums[i + 1]) {
        i--;
    }

    if (i >= 0) {
        // Step 2: Find the first index `j` from the end where nums[j] > nums[i]
        let j = nums.length - 1;
        while (j >= 0 && nums[j] <= nums[i]) {
            j--;
        }
        // Step 3: Swap nums[i] and nums[j]
        [nums[i], nums[j]] = [nums[j], nums[i]];
    }

    // Step 4: Reverse the suffix starting at i + 1
    let left = i + 1;
    let right = nums.length - 1;
    while (left < right) {
        [nums[left], nums[right]] = [nums[right], nums[left]];
        left++;
        right--;
    }
}

export function get_all_dihedral(n: number): number[][] {
    return generate_group([[1, 0], [0, 1]], (a: number[], b: number[]) => dihedral_multiply(a, b, n), array_eq, 0).sort(
        (a, b) => ((a[1] == b[1]) ? a[0] - b[0] : a[1] - b[1]))
}

export function dihedral_multiply(a: number[], b: number[], n: number) {
    let r_a = a[0]
    let s_a = a[1]
    let r_b = b[0]
    let s_b = b[1]
    if (s_a == 0) {
        return [(r_a + r_b) % n, s_b]
    } else {
        return [(r_a + n - r_b) % n, 1 - s_b]
    }
}

export function dihedral_to_permutation(dihedral: number[], n: number) {
    let perm: number[] = []
    let r: number[] = []
    for (let i = 2; i <= n; i++) {
        r.push(i)
    }
    r.push(1)

    for (let i = 0; i < dihedral[0]; i++) {
        perm = permutation_multiply(perm, r)
    }

    if (dihedral[1] == 1) {
        let s: number[] = []
        for (let i = 1; i <= n; i++) {
            if (i == 1) {
                s.push(i);
            } else {
                s.push(n + 2 - i)
            }
        }
        perm = permutation_multiply(perm, s)
    }

    return perm
}

export function dihedral_to_str(a: number[]) {
    let r = a[0]
    let s = a[1]
    if (r == 0 && s == 0) {
        return "e"
    }
    if (r == 0) {
        return "s"
    }
    return "r" + (r == 1 ? "" : get_sup(r.toString())) + (s == 0 ? "" : "s")
}

// relations
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

export function generate_group<T>(generators: T[],
                                  multiply: (a: T, b: T) => T,
                                  eq: (a: T, b: T) => boolean,
                                  limit: number): T[] {
    let ret: T[] = Array.from(generators)
    let last_length: number = 0
    let current_length: number = ret.length
    console.log("generating elements of the group, size from " + last_length + " to " + current_length)
    while (last_length < current_length) {
        let step = 20
        for (let i = 0; i < current_length; i++) {
            if (i % Math.max(1, Math.floor(current_length / step)) == 0) {
                console.debug("working on " + i + "/" + current_length)
            }
            for (let j = last_length; j < current_length; j++) {
                let product_ij = multiply(ret[i], ret[j])
                if (!ret.some(ele => eq(ele, product_ij))) {
                    console.log("adding " + i + ":" + ret[i] + " " + j + ":" + ret[j] + " " + product_ij)
                    ret.push(product_ij)
                }
            }
        }

        for (let i = 0; i < current_length; i++) {
            for (let j = last_length; j < current_length; j++) {
                let product_ji = multiply(ret[j], ret[i])
                if (!ret.some(ele => eq(ele, product_ji))) {
                    console.log("adding " + j + ":" + ret[j] + " " + i + ":" + ret[i] + " " + product_ji)
                    ret.push(product_ji)
                }
            }
        }


        last_length = current_length
        current_length = ret.length
        console.log("generating elements of the group, size from " + last_length + " to " + current_length)
        if (limit > 0 && current_length > limit) {
            break;
        }
    }

    return ret
}

export function transpose<T>(a: T[][]) {
    let m = a.length
    let n = a[0].length
    let ans: T[][] = []
    for (let i = 0; i < n; i++) {
        let array: T[] = []
        for (let j = 0; j < m; j++) {
            array.push(a[j][i])
        }
        ans.push(array)
    }
    return ans
}

export function matrix_multiply_general<T1, T2, T3>(
    a: T1[][],
    b: T2[][],
    multiply: ((a: T1, b: T2) => T3),
    addition: ((a: T3, b: T3) => T3)
): T3[][] {
    const rows_a = a.length;
    const cols_a = a[0].length;
    const rows_b = b.length;
    const cols_b = b[0].length;

    if (cols_a !== rows_b) {
        console.log(a, b)
        throw new Error("Matrix dimensions do not match for multiplication " + [rows_a, cols_a, rows_b, cols_a]);
    }

    const result: T3[][] = [];

    for (let i = 0; i < rows_a; i++) {
        const row: T3[] = [];
        for (let j = 0; j < cols_b; j++) {
            let sum: T3 = multiply(a[i][0], b[0][j])
            for (let k = 1; k < cols_a; k++) {
                const prod = multiply(a[i][k], b[k][j]);
                sum = addition(sum, prod);
            }
            row.push(sum);
        }
        result.push(row);
    }

    return result;
}

export function matrix_multiply_number(
    a: number[][],
    b: number[][]
): number[][] {
    return matrix_multiply_general(a, b, (m, n) => m * n, (a, b) => a + b);
}

export function matrix_multiply_zn(
    a: number[][],
    b: number[][],
    n: number
): number[][] {
    return matrix_multiply_general(a, b, get_multiply_mod_n_function(n), get_add_mod_n_function(n));
}

function get_det_func<T>(
    get: (i: number, j: number) => T,
    n: number,
    multiply: (a: T, b: T) => T,
    addition: (a: T, b: T) => T,
    add_inverse: (a: T) => T
): T {
    if (n === 1) return get(0, 0);

    if (n === 2) {
        return addition(
            multiply(get(0, 0), get(1, 1)),
            add_inverse(multiply(get(0, 1), get(1, 0)))
        );
    }

    let det = undefined as unknown as T;
    let first = true;

    for (let j = 0; j < n; j++) {
        const sub_get = (i: number, k: number) =>
            get(i + 1, k < j ? k : k + 1);
        const sign = (j % 2 === 0) ? (x: T) => x : add_inverse;
        const cofactor = multiply(
            sign(get(0, j)),
            get_det_func(sub_get, n - 1, multiply, addition, add_inverse)
        );

        if (first) {
            det = cofactor;
            first = false;
        } else {
            det = addition(det, cofactor);
        }
    }

    return det;
}

export function get_det<T>(
    a: T[][],
    multiply: (a: T, b: T) => T,
    addition: (a: T, b: T) => T,
    add_inverse: (a: T) => T
): T {
    const n = a.length;
    if (n === 0 || a[0].length !== n) {
        throw new Error("Matrix must be square");
    }

    const get = (i: number, j: number) => a[i][j];
    return get_det_func(get, n, multiply, addition, add_inverse);
}

export function get_inverse<T>(
    a: T[][],
    multiply: (a: T, b: T) => T,
    addition: (a: T, b: T) => T,
    add_inverse: (a: T) => T,
    mul_inverse: (a: T) => T
): T[][] {
    const n = a.length;
    if (n === 0 || a[0].length !== n) {
        throw new Error("Matrix must be square");
    }

    const get = (i: number, j: number) => a[i][j];
    const det = get_det_func(get, n, multiply, addition, add_inverse);
    const det_inv = mul_inverse(det);

    const adjugate: T[][] = Array.from({length: n}, () => Array<T>(n));

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            const sub_get = (r: number, c: number) => {
                const row = r < i ? r : r + 1;
                const col = c < j ? c : c + 1;
                return get(row, col);
            };
            const cofactor = get_det_func(
                sub_get, n - 1, multiply, addition, add_inverse
            );
            const sign = ((i + j) % 2 === 0) ? (x: T) => x : add_inverse;
            adjugate[j][i] = multiply(det_inv, sign(cofactor)); // Transposed
        }
    }

    return adjugate;
}


export function inner_product(a: number[], b: number[]) {
    let product = matrix_multiply_number([a], transpose([b]))
    return product[0][0]
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

export function array_to_matrix<T>(array: T[], m: number, n: number): T[][] {
    if (array.length !== m * n) {
        throw new Error(`Array length ${array.length} does not match matrix dimensions ${m}×${n}`);
    }

    const matrix: T[][] = [];
    for (let i = 0; i < m; i++) {
        const row: T[] = array.slice(i * n, (i + 1) * n);
        matrix.push(row);
    }
    return matrix;
}

export function gen_general_linear_n_zm(n: number, m: number): number[][][] {
    let gen: number[][] = []
    for (let j = 0; j < n * n; j++) {
        let elements: number[] = [];
        for (let i = 0; i < m; i++) {
            elements.push(i)
        }
        gen.push(elements)
    }

    return cartesian_product(gen).map(arr => array_to_matrix(arr, n, n)).filter(mat => get_det(mat, get_multiply_mod_n_function(m), get_add_mod_n_function(m), get_add_inverse_mod_n_function(m)) != 0)
}

function complex_to_matrix(complex: number[] | number) {
    if (typeof complex == "number") {
        return complex_to_matrix([complex, 0])
    }
    // TODO size check
    let real = complex[0]
    let imaginary = complex[1]
    return [[real, -imaginary], [imaginary, real]]
}

function matrix_to_complex(matrix: number[][]) {
    // TODO size check
    let real = matrix[0][0]
    let imaginary = matrix[1][0]
    return [real, imaginary]
}

export function complex_multiply(a: number[] | number, b: number[] | number) {
    let mat_a = complex_to_matrix(a)
    let mat_b = complex_to_matrix(b)
    console.log(mat_a, mat_b)
    return matrix_to_complex(matrix_multiply_number(mat_a, mat_b))
}