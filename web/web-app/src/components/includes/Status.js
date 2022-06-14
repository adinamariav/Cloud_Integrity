import React, { Component } from "react";
import {socket} from "./Context"
import { MDBDataTable } from 'mdbreact';

class Status extends Component {

    state = {
        cpu : 0,
        mem : 0,
        rows : [],
        columns : [
            {
                label: 'Host',
                field: 'host',
                width: 100
            },
            {
                label: 'Dir',
                field: 'dir',
                width: 30
            },
            {
                label: 'Last 2s',
                field: 'last2',
                width: 50
            },
            {
                label: 'Last 10s',
                field: 'last10',
                width: 50
            },
            {
                label: 'Last 40s',
                field: 'last40',
                width: 50
            },
            {
                label: 'Cumulative',
                field: 'cumul',
                width: 50
            },
    ]
    }

    componentDidMount() {
        socket.on("cpu", (load) => {
            this.setState({ cpu: load["data"]})
        })

        socket.on("mem", (load) => {
            this.setState({ mem: load["data"]})
        })

        socket.on("net", (stat) => {
            var network = this.state.rows
            var data = JSON.parse(stat["data"])

            if (network.length === 0)
            network.push(data)
            if (network[network.length - 1]["dir"] !== data["dir"])
                network.push(data)
            
            this.setState({ rows: network})
        })
    }
    
    render () {
        return(
                <div className="row" style={{marginTop:`2rem`}}>
                    <div className="card col" style={{backgroundColor:`#F6F5F5`}}>
                    <MDBDataTable
                                            disableRetreatAfterSorting={true}
                                            paging={false}
                                            searching={false}
                                            page
                                            scrollY
                                            small
                                            reactive
                                            maxHeight='15vh'
                                            fixed
                                            data={{ columns: this.state.columns, rows: this.state.rows }}
                                            style={{backgroundColor:`#F6F5F5`}}
                                            />
                    <p>CPU load: { this.state.cpu } Memory load: { this.state.mem } </p>
                    </div>
                </div>
        )
    }
}

export default Status;