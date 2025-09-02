export function clear_table(table: HTMLTableElement) {
    while (true) {
        if (table.rows.length == 0) {
            break
        }
        table.deleteRow(0)
    }
}

// Row, Column, Element
export function draw_table<R, C, E>(
    table: HTMLTableElement,
    rows: R[],
    cols: C[],
    multiply: (row: number, col: number) => E,
    rows_to_string: (a: R) => string,
    cols_to_string: (a: C) => string,
    element_to_string: (a: E) => string,
    row_get_color: (a: R) => string,
    col_get_color: (a: C) => string,
    element_get_color: (row: number, col: number) => string
) {
    clear_table(table)
    table.style.alignSelf = "center"
    table.style.borderStyle = "solid"
    table.style.textAlign = "center"
    {
        let row = table.insertRow();
        row.insertCell();
        for (let i = 0; i < cols.length; i++) {
            let cell = row.insertCell()

            cell.style.borderStyle = "solid"
            cell.innerHTML = cols_to_string(cols[i])
            cell.style.background = col_get_color(cols[i])

        }
    }

    for (let i = 0; i < rows.length; i++) {
        let row = table.insertRow()
        let cell = row.insertCell()
        cell.style.borderStyle = "solid"
        cell.style.background = row_get_color(rows[i])
        cell.innerHTML = rows_to_string(rows[i])
        for (let j = 0; j < cols.length; j++) {
            let cell_product = row.insertCell()
            cell_product.style.borderStyle = "solid"
            let element: E = multiply(i, j)
            cell_product.innerHTML = element_to_string(element)
            cell_product.style.background = element_get_color(i, j)
        }
    }
}

export function draw_multiplication_table<T>(
    table: HTMLTableElement,
    input: T[],
    multiply: (a: T, b: T) => T,
    to_string: (a: T) => string,
    input_get_color: (a: T) => string,
    product_get_color: (a: T, b: T, c: T) => string
) {
    draw_table(
        table,
        input,
        input,
        (i: number, j: number) => multiply(input[i], input[j]),
        to_string,
        to_string,
        to_string,
        input_get_color,
        input_get_color,
        (i: number, j: number) => product_get_color(input[i], input[j], multiply(input[i], input[j]))
    )
}

export function matrix_to_cell(arr: any[][]): string {
    const matrixHtml = arr.map(row =>
        `<div style="white-space: nowrap;">${row.map(x => x.toString()).join(' ')}</div>`
    ).join('');

    return `
    <div style="display: flex; font-family: monospace;">
        <div style="display: flex; flex-direction: column; justify-content: center;">(</div>
        <div style="margin: 0 4px;">
            ${matrixHtml}
        </div>
        <div style="display: flex; flex-direction: column; justify-content: center;">)</div>
    </div>
    `;
}