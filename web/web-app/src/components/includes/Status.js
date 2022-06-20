import React, { Component } from "react";
import {socket} from "./Context"
import BootstrapTable from 'react-bootstrap-table-next';
import filterFactory, { textFilter } from 'react-bootstrap-table2-filter';
import ScrollToBottom from 'react-scroll-to-bottom';
import { css } from '@emotion/css';
import './css/Status.css'

class Status extends Component {

    state = {
        cpu : 0,
        mem : 0,
        rows : [],
        columns : [
            {
                text: 'Time',
                dataField: 'time',
                headerStyle: { backgroundColor: 'white', whiteSpace:`normal`}
            },
            {
                text: 'Source',
                dataField: 'from',
                filter: textFilter(),
                headerStyle: { backgroundColor: 'white', whiteSpace:`normal`}
            },
            {
                text: 'Destination',
                dataField: 'to',
                filter: textFilter(),
                headerStyle: { backgroundColor: 'white'}
            },
            {
                text: 'Length',
                dataField: 'len',
                filter: textFilter(),
                headerStyle: { backgroundColor: 'white'}
            }
    ]}

    expandRow = {
        renderer: row => (
          <div>
            <p>{ row["details"] }</p>
          </div>
        ),
      };


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
            data["id"] = network.length + 1

            network.push(data)            
            this.setState({ rows: network })
        })
    }
    
    render () {
        const more_rows = css({
            height: `18rem`
        });
        
        const no_rows = css({
            height: `7rem`
        });
        
        let nrows = this.state.rows.length

        return(
                <div className="row card shadow" style={{ overflow:`scroll`}}>
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
                    <p>CPU load: { this.state.cpu } Memory load: { this.state.mem } </p>
                </div>
        )
    }
}

export default Status;