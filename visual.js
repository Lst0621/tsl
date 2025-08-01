// Row, Column, Element
export function draw_table(table, rows, cols, multiply, rows_to_string, cols_to_string, element_to_string, row_get_color, col_get_color, element_get_color) {
    while (true) {
        if (table.rows.length == 0) {
            break;
        }
        table.deleteRow(0);
    }
    table.style.alignSelf = "center";
    table.style.borderStyle = "solid";
    table.style.textAlign = "center";
    {
        let row = table.insertRow();
        row.insertCell();
        for (let i = 0; i < cols.length; i++) {
            let cell = row.insertCell();
            cell.style.borderStyle = "solid";
            cell.innerHTML = cols_to_string(cols[i]);
            cell.style.background = col_get_color(cols[i]);
        }
    }
    for (let i = 0; i < rows.length; i++) {
        let row = table.insertRow();
        let cell = row.insertCell();
        cell.style.borderStyle = "solid";
        cell.style.background = row_get_color(rows[i]);
        cell.innerHTML = rows_to_string(rows[i]);
        for (let j = 0; j < cols.length; j++) {
            let cell_product = row.insertCell();
            cell_product.style.borderStyle = "solid";
            let element = multiply(i, j);
            cell_product.innerHTML = element_to_string(element);
            cell_product.style.background = element_get_color(i, j);
        }
    }
}
export function draw_multiplication_table(table, input, multiply, to_string, input_get_color, product_get_color) {
    draw_table(table, input, input, (i, j) => multiply(input[i], input[j]), to_string, to_string, to_string, input_get_color, input_get_color, (i, j) => product_get_color(input[i], input[j], multiply(input[i], input[j])));
}
export function matrix_to_cell(arr) {
    const matrixHtml = arr.map(row => `<div style="white-space: nowrap;">${row.map(x => x.toString()).join(' ')}</div>`).join('');
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
