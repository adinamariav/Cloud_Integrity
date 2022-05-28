import React from 'react'
import { DataContext } from './Context'

export const CustomDropdown = (props) => (
  <div className="form-group">
    <strong>{props.name}</strong>
    <select
      className="form-control"
      name="{props.name}"
      onChange={props.onChange}
    >
      <option defaultValue>Select VM{props.name}</option>
      {props.options.map((item, index) => (
        <option key={index} value={item.id}>
          {item.name}
        </option>
      ))}
    </select>
  </div>
)

export default class CustomListDropDown extends React.Component {
  static contextType = DataContext;

  constructor() {
    super()
    this.state = {
      collection: [],
      value: '',
    }
  }

  componentDidMount() {
    fetch('/list/vm')
    .then((response) => response.json())
    .then((res) => this.setState({ collection: res }))
  }

  onChange = (event) => {
    this.setState({ value: event.target.value })
    const {collection} = this.state;
    this.context.selectVM(collection[event.target.value].name)
  }

  render() {
    return (
      <div className="container mt-4">
        <CustomDropdown
          name={this.state.username}
          options={this.state.collection}
          onChange={this.onChange}
        />
      </div>
    )
  }
}