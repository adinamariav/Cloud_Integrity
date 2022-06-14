import React, { Component } from "react";
import { DataContext } from "./Context";
import { MDBDataTable } from 'mdbreact';
import {socket} from "./Context"

class SyscallTable extends Component {

    static contextType = DataContext;

    state = {
        columns : [],
        rows : [],
        buffer : []
    }

    constructor() {
        super()
        this.state.columns = [
            {
            label: 'PID',
            field: 'pid',
            sort: 'asc',
            width: 90
            },
            {
            label: 'Name',
            field: 'name',
            sort: 'asc',
            width: 125
            },
            {
            label: 'RDI',
            field: 'rdi',
            sort: 'asc',
            width: 120
            },
            {
            label: 'RSI',
            field: 'rsi',
            sort: 'asc',
            width: 100
            },
            {
            label: 'RDX',
            field: 'rdx',
            sort: 'asc',
            width: 100
            },
            {
                label: 'R10',
                field: 'r10',
                sort: 'asc',
                width: 100
            },
            {
            label: 'R8',
            field: 'r8',
            sort: 'asc',
            width: 100
            },
            {
                label: 'R9',
                field: 'r9',
                sort: 'asc',
                width: 100
            }]
      }

      componentDidMount() {
        
        socket.on("syscall", (syscall) => {
            const rows = this.state.rows;
            const buffer = this.state.buffer;

            if (buffer.length == 5) {
                this.setState( { rows : [...rows, ...buffer] })
                this.setState( { buffer : [] })
            }
             else {
                var data = JSON.parse(syscall["data"])
                
                data.forEach((item) => {
                        buffer.push(item)
                })
                this.setState( { buffer : buffer })
             }
        })
      }

    render() {

        return (
            <div className="card shadow row" style={{marginTop:`2rem`}}>
                                    <div className="row card-body"  style={{overflow:`hidden`}}>
                                        <MDBDataTable
                                            disableRetreatAfterSorting={true}
                                            paging={false}
                                            page
                                            scrollY
                                            small
                                            reactive
                                            maxHeight='45vh'
                                            fixed
                                            data={{ columns: this.state.columns, rows: this.state.rows }}
                                            />
                                    </div>
                                </div>
        )
    }
};

export default SyscallTable
