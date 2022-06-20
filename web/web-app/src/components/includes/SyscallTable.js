import React, { Component } from "react";
import { DataContext } from "./Context";
import { MDBDataTable } from 'mdbreact';
import BootstrapTable from 'react-bootstrap-table-next';
import filterFactory, { textFilter } from 'react-bootstrap-table2-filter';
import {socket} from "./Context"
import ScrollToBottom from 'react-scroll-to-bottom';
import { css } from '@emotion/css';

class SyscallTable extends Component {

    static contextType = DataContext;

    state = {
        columns : [],
        rows : [],
        buffer : [],
        len: 0
    }

    constructor() {
        super()
        this.state.columns = [
            {
            text: 'PID',
            dataField: 'pid',
            filter: textFilter(),
            headerStyle: { backgroundColor: 'white', whiteSpace:`normal`}
            },
            {
            text: 'Name',
            dataField: 'name',
            filter: textFilter(),
            headerStyle: { backgroundColor: 'white', whiteSpace:`normal`}
            },
            {
            text: 'RDI',
            dataField: 'rdi',
            headerStyle: { backgroundColor: 'white', whiteSpace:`normal`}
            },
            {
            text: 'RSI',
            dataField: 'rsi',
            headerStyle: { backgroundColor: 'white', whiteSpace:`normal`}
            },
            {
            text: 'RDX',
            dataField: 'rdx',
            headerStyle: { backgroundColor: 'white', whiteSpace:`normal`}
            },
            {
            text: 'R10',
            dataField: 'r10',
            headerStyle: { backgroundColor: 'white', whiteSpace:`normal`}
            },
            {
            text: 'R8',
            dataField: 'r8',
            headerStyle: { backgroundColor: 'white', whiteSpace:`normal`}
            },
            {
            text: 'R9',
            dataField: 'r9',
            headerStyle: { backgroundColor: 'white', whiteSpace:`normal`}
            }]
      }

    expandRow = {
        renderer: row => (
          <div>
            <p style = {{ textAlign:`center` }} > { row["details"] } </p>
          </div>
        ),
      };

      componentDidMount() {
        socket.on("syscall", (syscall) => {
            const rows = this.state.rows;
            var data = JSON.parse(syscall["data"])
            var size = this.state.len

            var newRows = [...rows, ...data]

            if (newRows.length > 2000) {
                newRows.shift(data.length)
            }

            this.setState( { len: size + data.length} )
            this.setState( { rows : newRows })

            console.log(this.state.len)
        })
      }

    render() {

        const more_rows = css({
            height: `32rem`
        });
        
        const no_rows = css({
            height: `7rem`
        });

        let nrows = this.state.rows.length

        return (
            <div className="card shadow row" style={{marginTop:`2rem`}}>
                                    <ScrollToBottom className = { nrows > 0 ? more_rows : no_rows }>
                                        <BootstrapTable 
                                            keyField = 'id' 
                                            id = "table" 
                                            data = { this.state.rows } 
                                            columns = { this.state.columns } 
                                            filter = { filterFactory() }
                                            expandRow={ this.expandRow }
                                            />
                                    </ScrollToBottom>
                                </div>
        )
    }
};

export default SyscallTable
